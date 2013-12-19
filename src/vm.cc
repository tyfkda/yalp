//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#include "vm.hh"
#include "allocator.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"

#include <assert.h>
#include <iostream>
#include <string.h>  // for memmove

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
  OP(MACRO) \
  OP(EXPND) \
  OP(SHRNK) \

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

static inline void moveStackElems(Svalue* stack, int dst, int src, int n) {
  if (n > 0)
    memmove(&stack[dst], &stack[src], sizeof(Svalue) * n);
}

static bool checkArgNum(State* state, Svalue fn, int argNum, int min, int max) {
  if (argNum < min) {
    state->runtimeError("Too few arguments, %@ requires at least %d but %d", &fn, min, argNum);
    return false;
  } else if (max >= 0 && argNum > max) {
    state->runtimeError("Too many arguments, %@ requires at most %d but %d", &fn, max, argNum);
    return false;
  }
  return true;
}

// Box class.
class Box : public Sobject {
public:
  Box(Svalue x)
    : Sobject()
    , x_(x) {}
  virtual Type getType() const override  { return TT_BOX; }

  void set(Svalue x)  { x_ = x; }
  Svalue get()  { return x_; }

  virtual void output(State* state, Stream* o, bool inspect) const override {
    // This should not be output, but debug purpose.
    o->write("#<box ");
    x_.output(state, o, inspect);
    o->write('>');
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

Svalue Vm::run(Svalue code) {
  Svalue nil = Svalue::NIL;
  a_ = nil;
  x_ = code;
  c_ = nil;
  f_ = 0;
  return runLoop();
}

Svalue Vm::runLoop() {
 again:
  if (trace_) {
    FileStream out(stdout);
    std::cout << "run: stack=" << s_ << ", x=";
    x_.output(state_, &out, true);
    std::cout << std::endl;
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
      if (!exist)
        state_->runtimeError("Unbound `%@`", &sym);
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
      if (!assignGlobal(sym, a_))
        state_->runtimeError("Global variable `%@` not defined", &sym);
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
      if (!a_.isObject() || !a_.toObject()->isCallable())
        state_->runtimeError("Can't call `%@`", &a_);

      switch (a_.getType()) {
      case TT_CLOSURE:
        {
          Closure* closure = static_cast<Closure*>(a_.toObject());
          int min = closure->getMinArgNum(), max = closure->getMaxArgNum();
          checkArgNum(state_, a_, argNum, min, max);
          pushCallStack(closure);

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
          NativeFunc* native = static_cast<NativeFunc*>(a_.toObject());
          int min = native->getMinArgNum(), max = native->getMaxArgNum();
          checkArgNum(state_, a_, argNum, min, max);
          pushCallStack(native);

          f_ = s_;
          s_ = push(state_->fixnumValue(argNum), s_);
          x_ = return_;
          a_ = native->call(state_);
        }
        break;
      case TT_CONTINUATION:
        {
          Continuation* continuation = static_cast<Continuation*>(a_.toObject());
          checkArgNum(state_, a_, argNum, 0, 1);
          pushCallStack(continuation);
          a_ = (argNum == 0) ? Svalue::NIL : index(s_, 0);

          int savedStackSize = continuation->getStackSize();
          const Svalue* savedStack = continuation->getStack();
          memcpy(stack_, savedStack, sizeof(Svalue) * savedStackSize);
          s_ = savedStackSize;

          int callStackSize = continuation->getCallStackSize();
          if (callStackSize == 0)
            callStack_.clear();
          else {
            const CallStack* callStack = continuation->getCallStack();
            callStack_.resize(callStackSize);
            memcpy(&callStack_[0], callStack, sizeof(CallStack) * callStackSize);
          }

          // do-return
          x_ = index(s_, 0);
          f_ = index(s_, 1).toFixnum();
          c_ = index(s_, 2);
          s_ -= 3;
          //popCallStack();
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
      int s = s_;
      if (state_->isTrue(tail)) {
        int calleeArgNum = index(s_, 0).toFixnum();
        s -= calleeArgNum + 1;
      }
      a_ = createContinuation(s);
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
  case EXPND:
    {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      expandFrame(n);
    }
    goto again;
  case SHRNK:
    {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      shrinkFrame(n);
    }
    goto again;
  default:
    state_->runtimeError("Unknown op `%@`", &op);
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

Svalue Vm::createContinuation(int s) {
  Allocator* allocator = state_->getAllocator();
  void* memory = OBJALLOC(allocator, sizeof(Continuation));
  return Svalue(new(memory) Continuation(allocator, stack_, s,
                                         &callStack_[0], callStack_.size()));
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
  moveStackElems(stack_, s - n - m - 1, s - n, n);
  return s - m - 1;
}

int Vm::modifyRestParams(int argNum, int minArgNum, int s) {
  Svalue rest = createRestParams(argNum, minArgNum, s);
  int ds = 0;
  if (argNum <= minArgNum) {
    // No rest param space: move all args +1 to create space.
    ds = minArgNum - argNum + 1;  // This must be 1 if arguments count is checked, but just in case.
    reserveStack(s + ds);
    moveStackElems(stack_, s - argNum + ds, s - argNum, argNum);
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

void Vm::expandFrame(int n) {
  if (f_ == 0) {
    // No upper frame: create first frame.
    //   Before: f_[z][y][x][B][A]s_
    //   After:  [B][A]f_[N][z][x][y]s_
    reserveStack(s_ + n + 1);
    moveStackElems(stack_, n + 1, 0, s_);  // Shift stack.
    moveStackElems(stack_, 0, s_ + 1, n);  // Move arguments.
    stack_[n] = state_->fixnumValue(n);
    s_ += 1;
    f_ = n;
    return;
  }

  // Before: [z][y][x]f_[argnum][B][A]s_
  // After:  [z][y][x][B][A]f_[argnum+n]s_
  int argNum = stack_[f_].toFixnum();
  int src = f_, dst = src + n;
  reserveStack(s_ + n);
  stack_[f_] = state_->fixnumValue(argNum + n);
  moveStackElems(stack_, dst, src, s_ - src);  // Shift stack.
  moveStackElems(stack_, src, s_, n);  // Move arguments.
  f_ += n;
}

void Vm::shrinkFrame(int n) {
  if (f_ - n == 0) {
    assert(stack_[f_].toFixnum() == n);
    // No upper frame: create first frame.
    //   Before: [B][A]f_[N][z][x][y]s_
    //   After:  f_[z][y][x]s_
    moveStackElems(stack_, 0, f_ + 1, s_ - (f_ + 1));  // Shift stack.
    s_ -= f_ + 1;
    f_ = 0;
    return;
  }

  // Before: [z][y][x][B][A]f_[argnum]s_
  // After:  [z][y][x]f_[argnum-n]s_
  int argnum = stack_[f_].toFixnum();
  stack_[f_] = state_->fixnumValue(argnum - n);
  int src = f_, dst = f_ - n;
  moveStackElems(stack_, dst, src, s_ - src);  // Shift stack.
  s_ -= n;
  f_ -= n;
}

Svalue Vm::referGlobal(Svalue sym, bool* pExist) {
  const Svalue* result = globalVariableTable_->get(sym);
  if (pExist != NULL)
    *pExist = result != NULL;
  return result != NULL ? *result : Svalue::NIL;
}

void Vm::defineGlobal(Svalue sym, Svalue value) {
  state_->checkType(sym, TT_SYMBOL);
  globalVariableTable_->put(sym, value);

  if (value.isObject() && value.toObject()->isCallable()) {
    const Symbol* name = sym.toSymbol(state_);
    static_cast<Callable*>(value.toObject())->setName(name);
  }
}

bool Vm::assignGlobal(Svalue sym, Svalue value) {
  state_->checkType(sym, TT_SYMBOL);
  if (globalVariableTable_->get(sym) == NULL)
    return false;

  globalVariableTable_->put(sym, value);
  return true;
}

int Vm::getArgNum() const {
  return index(f_, -1).toFixnum();
}

Svalue Vm::getArg(int index) const {
  return this->index(f_, index);
}

Svalue Vm::funcall(Svalue fn, int argNum, const Svalue* args) {
  switch (fn.getType()) {
  case TT_CLOSURE:
  case TT_CONTINUATION:
    funcallSetup(fn, argNum, args, false);
    return runLoop();
  default:
    return funcallSetup(fn, argNum, args, false);
  }
}

Svalue Vm::tailcall(Svalue fn, int argNum, const Svalue* args) {
  shiftCallStack();
  return funcallSetup(fn, argNum, args, true);
}

Svalue Vm::funcallSetup(Svalue fn, int argNum, const Svalue* args, bool tailcall) {
  if (!fn.isObject() || !fn.toObject()->isCallable()) {
    state_->runtimeError("Can't call `%@`", &fn);
    return Svalue::NIL;
  }

  switch (fn.getType()) {
  case TT_CLOSURE:
    {
      bool isTail = CAR(x_).eq(opcodes_[RET]);

      if (isTail) {
        // Shifts arguments.
        int calleeArgNum = index(f_, -1).toFixnum();
        s_ = pushArgs(argNum, args, s_);
        s_ = shiftArgs(argNum, calleeArgNum, s_);
        shiftCallStack();
      } else {
        // Makes frame.
        s_ = push(x_, push(state_->fixnumValue(s_), push(c_, s_)));
        s_ = pushArgs(argNum, args, s_);
      }

      Closure* closure = static_cast<Closure*>(fn.toObject());
      int min = closure->getMinArgNum(), max = closure->getMaxArgNum();
      checkArgNum(state_, fn, argNum, min, max);
      pushCallStack(closure);

      int ds = 0;
      if (closure->hasRestParam())
        ds = modifyRestParams(argNum, min, s_);
      s_ += ds;
      argNum += ds;
      x_ = closure->getBody();
      f_ = s_;
      s_ = push(state_->fixnumValue(argNum), s_);
      a_ = c_ = fn;
      // runLoop will run after this function exited.
    }
    return Svalue::NIL;
  case TT_NATIVEFUNC:
    {
      NativeFunc* native = static_cast<NativeFunc*>(fn.toObject());
      int min = native->getMinArgNum(), max = native->getMaxArgNum();
      checkArgNum(state_, fn, argNum, min, max);
      pushCallStack(native);

      // No frame.
      f_ = pushArgs(argNum, args, s_);
      s_ = push(state_->fixnumValue(argNum), f_);
      Svalue result = native->call(state_);

      if (tailcall)
        shiftCallStack();
      else
        popCallStack();

      s_ -= argNum + 1;
      return result;
    }
  case TT_CONTINUATION:
    {
      Continuation* continuation = static_cast<Continuation*>(fn.toObject());
      checkArgNum(state_, fn, argNum, 0, 1);
      pushCallStack(continuation);
      a_ = (argNum == 0) ? Svalue::NIL : args[0];

      int savedStackSize = continuation->getStackSize();
      const Svalue* savedStack = continuation->getStack();
      memcpy(stack_, savedStack, sizeof(Svalue) * savedStackSize);
      s_ = savedStackSize;

      // do-return
      x_ = index(s_, 0);
      f_ = index(s_, 1).toFixnum();
      c_ = index(s_, 2);
      s_ -= 3;
      // runLoop will run after this function exited.
    }
    return Svalue::NIL;
  default:
    assert(!"Must not happen");
    return Svalue::NIL;
  }
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
  assert(!callStack_.empty());
  callStack_.pop_back();
  while (!callStack_.empty() && callStack_[callStack_.size() - 1].isTailCall)
    callStack_.pop_back();
}

void Vm::shiftCallStack() {
  size_t n = callStack_.size();
  assert(n > 0);
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

  if (name.getType() != TT_SYMBOL)
    state_->runtimeError("Must be symbol, but `%@`", &name);
  macroTable_->put(name, closure);
}

}  // namespace yalp
