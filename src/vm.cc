//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#include "vm.hh"
#include "allocator.hh"
#include "yalp/object.hh"
#include "yalp/util.hh"
#include <alloca.h>
#include <assert.h>
#include <iostream>

namespace yalp {

//=============================================================================

#define OPS \
  OP(HALT) \
  OP(UNDEF) \
  OP(CONST) \
  OP(LREF) \
  OP(FREF) \
  OP(GREF) \
  OP(LSET) \
  OP(FSET) \
  OP(GSET) \
  OP(DEF) \
  OP(PUSH) \
  OP(TEST) \
  OP(CLOSE) \
  OP(FRAME) \
  OP(APPLY) \
  OP(RET) \
  OP(SHIFT) \
  OP(BOX) \
  OP(UNBOX) \
  OP(CONTI) \
  OP(NUATE) \
  OP(MACRO) \

enum Opcode {
#define OP(name)  name,
  OPS
#undef OP
  NUMBER_OF_OPCODE
};

#define CELL(x)  (static_cast<Cell*>(x.toObject()))
#define CAR(x)  (CELL(x)->car())
#define CDR(x)  (CELL(x)->cdr())
#define CADR(x)  (CAR(CDR(x)))
#define CDDR(x)  (CDR(CDR(x)))
#define CADDR(x)  (CAR(CDDR(x)))
#define CDDDR(x)  (CDR(CDDR(x)))

//=============================================================================

// Box class.
class Box : public Sobject {
public:
  Box(Svalue x)
    : Sobject()
    , x_(x) {}
  virtual Type getType() const override  { return TT_BOX; }

  void set(Svalue x)  { x_ = x; }
  Svalue get()  { return x_; }

  virtual void output(State* state, std::ostream& o, bool inspect) const override {
    // This should not be output, but debug purpose.
    o << "#<box ";
    x_.output(state, o, inspect);
    o << ">";
  }

protected:
  Svalue x_;
};

//=============================================================================

Vm* Vm::create(State* state) {
  void* memory = ALLOC(state->getAllocator(), sizeof(Vm));
  return new(memory) Vm(state);
}

void Vm::release() {
  Allocator* allocator = state_->getAllocator();
  this->~Vm();
  FREE(allocator, this);
}

Vm::~Vm() {
  if (stack_ != NULL)
    FREE(state_->getAllocator(), stack_);
  FREE(state_->getAllocator(), opcodes_);
}

Vm::Vm(State* state)
  : state_(state)
  , stack_(NULL), stackSize_(0)
  , trace_(false)
  , jmp_(NULL)
  , callStack_() {
  a_ = c_ = Svalue::NIL;
  x_ = endOfCode_;
  f_ = s_ = 0;

  {
    static const char* NameTable[NUMBER_OF_OPCODE] = {
#define OP(name)  #name,
      OPS
#undef OP
    };

    void* memory = ALLOC(state_->getAllocator(), sizeof(Svalue) * NUMBER_OF_OPCODE);
    opcodes_ = new(memory) Svalue[NUMBER_OF_OPCODE];
    for (int i = 0; i < NUMBER_OF_OPCODE; ++i)
      opcodes_[i] = state_->intern(NameTable[i]);
  }

  {
    Svalue ht = state_->createHashTable();
    assert(ht.getType() == TT_HASH_TABLE);
    globalVariableTable_ = static_cast<SHashTable*>(ht.toObject());
  }

  {
    Svalue ht = state_->createHashTable();
    macroTable_ = static_cast<SHashTable*>(ht.toObject());
  }

  endOfCode_ = list(state_, opcodes_[HALT]);
  return_ = list(state_, opcodes_[RET]);
}

void Vm::setTrace(bool b) {
  trace_ = b;
}

jmp_buf* Vm::setJmpbuf(jmp_buf* jmp) {
  jmp_buf* old = jmp_;
  jmp_ = jmp;
  return old;
}

void Vm::longJmp() {
  if (jmp_ != NULL) {
    longjmp(*jmp_, 1);
  }

  // If process comes here, something wrong.
  std::cerr << "Vm::longJmp failed" << std::endl;
  exit(1);
}

void Vm::markRoot() {
  globalVariableTable_->mark();
  macroTable_->mark();
  // Mark stack.
  for (int n = s_, i = 0; i < n; ++i)
    stack_[i].mark();
  // Mark a registers.
  a_.mark();
  c_.mark();
  x_.mark();
  endOfCode_.mark();
  return_.mark();
}

void Vm::reportDebugInfo() const {
  std::cout << "Global variables:" << std::endl;
  std::cout << "  capacity: #" << globalVariableTable_->getCapacity() << std::endl;
  std::cout << "  entry:    #" << globalVariableTable_->getEntryCount() << std::endl;
  std::cout << "  conflict: #" << globalVariableTable_->getConflictCount() << std::endl;
  std::cout << "  maxdepth: #" << globalVariableTable_->getMaxDepth() << std::endl;
}

void Vm::defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum) {
  void* memory = OBJALLOC(state_->getAllocator(), sizeof(NativeFunc));
  NativeFunc* nativeFunc = new(memory) NativeFunc(func, minArgNum, maxArgNum);
  defineGlobal(state_->intern(name), Svalue(nativeFunc));
}

Svalue Vm::getMacro(Svalue name) {
  const Svalue* result = macroTable_->get(name);
  return result != NULL ? *result : Svalue::NIL;
}

bool Vm::run(Svalue code, Svalue* pResult) {
  jmp_buf* old = NULL;
  jmp_buf jmp;
  bool ret = false;
  if (setjmp(jmp) == 0) {
    old = setJmpbuf(&jmp);
    Svalue nil = Svalue::NIL;
    a_ = nil;
    x_ = code;
    c_ = nil;
    f_ = 0;
    Svalue result = runLoop();
    if (pResult != NULL)
      *pResult = result;
    ret = true;
  }
  setJmpbuf(old);
  return ret;
}

Svalue Vm::runLoop() {
 again:
  if (trace_) {
    std::cout << "run: stack=" << s_ << ", x="; x_.output(state_, std::cout, true); std::cout << std::endl;
  }

  Svalue prex = x_;
  Svalue op = CAR(prex);
  x_ = CDR(prex);
  int opidx = findOpcode(op);
  switch (opidx) {
  case HALT:
    x_ = endOfCode_;
    return a_;
  case UNDEF:
    a_ = Svalue::NIL;
    goto again;
  case CONST:
    a_ = CAR(x_);
    x_ = CDR(x_);
    goto again;
  case LREF:
    {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      a_ = index(f_, n);
    }
    goto again;
  case FREF:
    {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      assert(c_.getType() == TT_CLOSURE);
      a_ = static_cast<Closure*>(c_.toObject())->getFreeVariable(n);
    }
    goto again;
  case GREF:
    {
      Svalue sym = CAR(x_);
      x_ = CDR(x_);
      assert(sym.getType() == TT_SYMBOL);
      bool exist;
      Svalue aa = referGlobal(sym, &exist);
      if (!exist) {
        sym.output(state_, std::cerr, true);
        std::cerr << ": ";
        state_->runtimeError("Unbound");
      }
      a_ = aa;
    }
    goto again;
  case LSET:
    {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      Svalue box = index(f_, n);
      assert(box.getType() == TT_BOX);
      static_cast<Box*>(box.toObject())->set(a_);
    }
    goto again;
  case FSET:
    {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      assert(c_.getType() == TT_CLOSURE);
      Svalue box = static_cast<Closure*>(c_.toObject())->getFreeVariable(n);
      assert(box.getType() == TT_BOX);
      static_cast<Box*>(box.toObject())->set(a_);
    }
    goto again;
  case GSET:
    {
      Svalue sym = CAR(x_);
      x_ = CDR(x_);
      assert(sym.getType() == TT_SYMBOL);
      if (!assignGlobal(sym, a_)) {
        sym.output(state_, std::cerr, true);
        state_->runtimeError(": Global variable not defined");
      }
    }
    goto again;
  case DEF:
    {
      Svalue sym = CAR(x_);
      x_ = CDR(x_);
      assert(sym.getType() == TT_SYMBOL);
      defineGlobal(sym, a_);
    }
    goto again;
  case PUSH:
    s_ = push(a_, s_);
    goto again;
  case TEST:
    {
      Svalue thn = CAR(x_);
      Svalue els = CDR(x_);
      x_ = state_->isTrue(a_) ? thn : els;
    }
    goto again;
  case CLOSE:
    {
      Svalue nparam = CAR(x_);  // Fixnum (fixed parameters function) or Cell (arbitrary number of parameters function).
      int nfree = CADR(x_).toFixnum();
      Svalue body = CADDR(x_);
      x_ = CDDDR(x_);
      int min, max;
      if (nparam.getType() == TT_CELL) {
        min = CAR(nparam).toFixnum();
        max = CADR(nparam).toFixnum();
      } else {
        min = max = nparam.toFixnum();
      }
      a_ = createClosure(body, nfree, s_, min, max);
      s_ -= nfree;

      if (nfree == 0) {
        // If no free variable, the closure has no reference to environment.
        // So bytecode can be replaced to reuse it.
        // (CLOSE nparam nfree body ...) => (CONST closure ...)
        CELL(prex)->setCar(opcodes_[CONST]);
        CELL(CDR(prex))->setCar(a_);
        CELL(CDR(prex))->setCdr(x_);
      }
    }
    goto again;
  case FRAME:
    {
      Svalue ret = CDR(x_);
      x_ = CAR(x_);
      s_ = push(ret, push(state_->fixnumValue(f_), push(c_, s_)));
    }
    goto again;
  case APPLY:
    {
      int argNum = CAR(x_).toFixnum();
      if (!a_.isObject() || !a_.toObject()->isCallable()) {
        state_->runtimeError("Can't call");
      }

      pushCallStack(static_cast<Callable*>(a_.toObject()));

      switch (a_.getType()) {
      case TT_CLOSURE:
        {
          Closure* closure = static_cast<Closure*>(a_.toObject());
          int min = closure->getMinArgNum(), max = closure->getMaxArgNum();
          if (argNum < min)
            state_->runtimeError("Too few arguments");
          else if (max >= 0 && argNum > max)
            state_->runtimeError("Too many arguments");

          int ds = 0;
          if (closure->hasRestParam())
            ds = modifyRestParams(argNum, min, s_);
          s_ += ds;
          argNum += ds;
          x_ = closure->getBody();
          f_ = s_;
          c_ = a_;
          s_ = push(state_->fixnumValue(argNum), s_);
        }
        break;
      case TT_NATIVEFUNC:
        {
          f_ = s_;
          s_ = push(state_->fixnumValue(argNum), s_);
          x_ = return_;

          NativeFunc* native = static_cast<NativeFunc*>(a_.toObject());
          a_ = native->call(state_, argNum);
        }
        break;
      default:
        assert(!"Must not happen");
        break;
      }
    }
    goto again;
  case RET:
    {
      int argNum = index(s_, 0).toFixnum();
      s_ -= argNum + 1;
      x_ = index(s_, 0);
      f_ = index(s_, 1).toFixnum();
      c_ = index(s_, 2);
      s_ -= 3;
      popCallStack();
    }
    goto again;
  case SHIFT:
    {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      int calleeArgNum = index(f_, -1).toFixnum();
      s_ = shiftArgs(n, calleeArgNum, s_);
      shiftCallStack();
    }
    goto again;
  case BOX:
    {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      indexSet(f_, n, box(index(f_, n)));
    }
    goto again;
  case UNBOX:
    assert(a_.getType() == TT_BOX);
    a_ = static_cast<Box*>(a_.toObject())->get();
    goto again;
  case CONTI:
    {
      Svalue tail = CAR(x_);
      x_ = CDR(x_);
      a_ = createContinuation(s_, tail);
    }
    goto again;
  case NUATE:
    {
      Svalue tail = CAR(x_);
      Svalue stack = CADR(x_);
      int argNum = index(f_, -1).toFixnum();
      a_ = (argNum == 0) ? Svalue::NIL : index(f_, 0);
      s_ = restoreStack(stack);

      if (state_->isTrue(tail)) {
        int calleeArgNum = index(s_, 0).toFixnum();
        s_ -= calleeArgNum + 1;
      }

      // do-return
      x_ = index(s_, 0);
      f_ = index(s_, 1).toFixnum();
      c_ = index(s_, 2);
      s_ -= 3;
      //popCallStack();
    }
    goto again;
  case MACRO:
    {
      Svalue name = CAR(x_);
      Svalue nparam = CADR(x_);
      Svalue body = CADDR(x_);
      x_ = CDDDR(x_);
      int min, max;
      if (nparam.getType() == TT_CELL) {
        min = CAR(nparam).toFixnum();
        max = CADR(nparam).toFixnum();
      } else {
        min = max = nparam.toFixnum();
      }
      registerMacro(name, min, max, body);
    }
    goto again;
  default:
    op.output(state_, std::cerr, true);
    std::cerr << ": ";
    state_->runtimeError("Unknown op");
    return a_;
  }
}

int Vm::findOpcode(Svalue op) {
  for (int i = 0; i < NUMBER_OF_OPCODE; ++i)
    if (op.eq(opcodes_[i]))
      return i;
  return -1;
}

Svalue Vm::createClosure(Svalue body, int nfree, int s, int minArgNum, int maxArgNum) {
  void* memory = OBJALLOC(state_->getAllocator(), sizeof(Closure));
  Closure* closure = new(memory) Closure(state_, body, nfree, minArgNum, maxArgNum);
  for (int i = 0; i < nfree; ++i)
    closure->setFreeVariable(i, index(s, i));
  return Svalue(closure);
}

Svalue Vm::createContinuation(int s, Svalue tail) {
  Svalue body = list(state_,
                     opcodes_[NUATE], tail,
                     saveStack(s));
  return createClosure(body, s, s, 0, 1);
}

Svalue Vm::box(Svalue x) {
  void* memory = OBJALLOC(state_->getAllocator(), sizeof(Box));
  return Svalue(new(memory) Box(x));
}

int Vm::push(Svalue x, int s) {
  reserveStack(s + 1);
  stack_[s] = x;
  return s + 1;
}

void Vm::reserveStack(int n) {
  if (n <= stackSize_)
    return;

  int newSize = n + 16;
  void* memory = REALLOC(state_->getAllocator(), stack_, sizeof(Svalue) * newSize);
  Svalue* newStack = static_cast<Svalue*>(memory);
  stack_ = newStack;
  stackSize_ = newSize;
}

Svalue Vm::index(int s, int i) const {
  assert(s - i - 1 >= 0);
  return stack_[s - i - 1];
}

void Vm::indexSet(int s, int i, Svalue v) {
  assert(s - i - 1 >= 0);
  stack_[s - i - 1] = v;
}

int Vm::shiftArgs(int n, int m, int s) {
  for (int i = n; --i >= 0; ) {
    indexSet(s, i + m + 1, index(s, i));
  }
  return s - m - 1;
}

int Vm::modifyRestParams(int argNum, int minArgNum, int s) {
  Svalue rest = createRestParams(argNum, minArgNum, s);
  int ds = 0;
  if (argNum <= minArgNum) {
    unshiftArgs(argNum, s);
    ds = 1;
  }
  indexSet(s + ds, minArgNum, rest);
  return ds;
}

Svalue Vm::createRestParams(int argNum, int minArgNum, int s) {
  Svalue acc = Svalue::NIL;
  for (int i = argNum; --i >= minArgNum; )
    acc = state_->cons(index(s, i), acc);
  return acc;
}

void Vm::unshiftArgs(int argNum, int s) {
  for (int i = 0; i < argNum; ++i)
    indexSet(s, i - 1, index(s, i));
}

Svalue Vm::referGlobal(Svalue sym, bool* pExist) {
  const Svalue* result = globalVariableTable_->get(sym);
  if (pExist != NULL)
    *pExist = result != NULL;
  return result != NULL ? *result : Svalue::NIL;
}

void Vm::defineGlobal(Svalue sym, Svalue value) {
  if (sym.getType() != TT_SYMBOL) {
    std::cerr << sym << ": ";
    state_->runtimeError("Must be symbol");
  }
  globalVariableTable_->put(sym, value);

  if (value.isObject() && value.toObject()->isCallable()) {
    const Symbol* name = sym.toSymbol(state_);
    static_cast<Callable*>(value.toObject())->setName(name);
  }
}

bool Vm::assignGlobal(Svalue sym, Svalue value) {
  if (sym.getType() != TT_SYMBOL ||
      globalVariableTable_->get(sym) == NULL)
    return false;

  globalVariableTable_->put(sym, value);
  return true;
}

Svalue Vm::saveStack(int s) {
  Allocator* allocator = state_->getAllocator();
  void* memory = OBJALLOC(allocator, sizeof(Vector));
  Vector* v = new(memory) Vector(allocator, s);
  for (int i = 0; i < s; ++i)
    v->set(i, stack_[i]);
  return Svalue(v);
}

int Vm::restoreStack(Svalue v) {
  assert(v.getType() == TT_VECTOR);
  Vector* vv = static_cast<Vector*>(v.toObject());
  int s = vv->size();
  for (int i = 0; i < s; ++i)
    stack_[i] = vv->get(i);
  return s;
}

int Vm::getArgNum() const {
  return index(f_, -1).toFixnum();
}

Svalue Vm::getArg(int index) const {
  return this->index(f_, index);
}

bool Vm::funcall(Svalue fn, int argNum, const Svalue* args, Svalue* pResult) {
  jmp_buf* old = NULL;
  jmp_buf jmp;
  bool ret = false;
  if (setjmp(jmp) == 0) {
    old = setJmpbuf(&jmp);
    Svalue result = funcallExec(fn, argNum, args);
    if (pResult != NULL)
      *pResult = result;
    ret = true;
  }
  setJmpbuf(old);
  return ret;
}

Svalue Vm::funcallExec(Svalue fn, int argNum, const Svalue* args) {
  switch (fn.getType()) {
  case TT_CLOSURE:
    tailcall(fn, argNum, args);
    return runLoop();
  default:
    return tailcall(fn, argNum, args);
  }
}

Svalue Vm::tailcall(Svalue fn, int argNum, const Svalue* args) {
  if (!fn.isObject() || !fn.toObject()->isCallable()) {
    fn.output(state_, std::cerr, true);
    state_->runtimeError("Can't call");
    return Svalue::NIL;
  }

  pushCallStack(static_cast<Callable*>(fn.toObject()));

  Svalue result;

  switch (fn.getType()) {
  case TT_CLOSURE:
    {
      bool isTail = CAR(x_).eq(opcodes_[RET]);

      if (isTail) {
        // Shifts arguments.
        int calleeArgNum = index(f_, -1).toFixnum();
        s_ = pushArgs(argNum, args, s_);
        s_ = shiftArgs(argNum, calleeArgNum, s_);
        // TODO: Confirm callstack is consistent.
      } else {
        // Makes frame.
        s_ = push(x_, push(state_->fixnumValue(s_), push(c_, s_)));
        s_ = pushArgs(argNum, args, s_);
      }

      Closure* closure = static_cast<Closure*>(fn.toObject());
      int min = closure->getMinArgNum(), max = closure->getMaxArgNum();
      if (argNum < min)
        state_->runtimeError("Too few arguments");
      else if (max >= 0 && argNum > max)
        state_->runtimeError("Too many arguments");

      int ds = 0;
      if (closure->hasRestParam())
        ds = modifyRestParams(argNum, min, s_);
      s_ += ds;
      argNum += ds;
      x_ = closure->getBody();
      f_ = s_;
      s_ = push(state_->fixnumValue(argNum), s_);
      a_ = c_ = fn;
      //result = runLoop();
      // runLoop will run after this function exited.

      shiftCallStack();
    }
    break;
  case TT_NATIVEFUNC:
    {
      // No frame.
      f_ = pushArgs(argNum, args, s_);
      s_ = push(state_->fixnumValue(argNum), f_);

      // Store current state in member variable for native function call.
      NativeFunc* native = static_cast<NativeFunc*>(fn.toObject());
      result = native->call(state_, argNum);

      popCallStack();

      s_ -= argNum + 1;
    }
    break;
  default:
    assert(!"Must not happen");
    break;
  }

  return result;
}

void Vm::resetError() {
  Svalue nil = Svalue::NIL;
  a_ = nil;
  x_ = endOfCode_;
  c_ = nil;
  f_ = s_ = 0;
  callStack_.clear();
}

int Vm::pushArgs(int argNum, const Svalue* args, int s) {
  for (int i = argNum; --i >= 0; )
    s = push(args[i], s);
  return s;
}

void Vm::pushCallStack(Callable* callable) {
  CallStack s = {
    callable,
    false,
  };
  callStack_.push_back(s);
}

void Vm::popCallStack() {
  callStack_.pop_back();
  while (!callStack_.empty() && callStack_[callStack_.size() - 1].isTailCall)
    callStack_.pop_back();
}

void Vm::shiftCallStack() {
  size_t n = callStack_.size();
  if (n >= 2 && callStack_[n - 2].callable == callStack_[n - 1].callable &&
      callStack_[n - 2].isTailCall) {
    // Self tail recursive case: eliminate call stack.
    callStack_.pop_back();
  } else {
    callStack_[n - 1].isTailCall = true;
  }
}

void Vm::registerMacro(Svalue name, int minParam, int maxParam, Svalue body) {
  Svalue closure = createClosure(body, 0, s_, minParam, maxParam);

  assert(name.getType() == TT_SYMBOL);
  static_cast<Closure*>(closure.toObject())->setName(name.toSymbol(state_));
  defineGlobal(name, state_->intern("*macro*"));

  if (name.getType() != TT_SYMBOL) {
    std::cerr << name << ": ";
    state_->runtimeError("Must be symbol");
  }
  macroTable_->put(name, closure);
}

}  // namespace yalp
