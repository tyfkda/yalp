//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#include "vm.hh"
#include "allocator.hh"
#include "yalp/object.hh"
#include <assert.h>
#include <iostream>

namespace yalp {

//=============================================================================

enum Opcode {
  HALT,
  UNDEF,
  CONST,
  LREF,
  FREF,
  GREF,
  LSET,
  FSET,
  GSET,
  PUSH,
  TEST,
  CLOSE,
  FRAME,
  APPLY,
  RET,
  SHIFT,
  BOX,
  UNBOX,
  CONTI,
  NUATE,
  MACRO,

  NUMBER_OF_OPCODE
};

#define CELL(x)  (static_cast<Cell*>(x.toObject()))
#define CAR(x)  (CELL(x)->car())
#define CDR(x)  (CELL(x)->cdr())
#define CADR(x)  (CAR(CDR(x)))
#define CDDR(x)  (CDR(CDR(x)))
#define CADDR(x)  (CAR(CDDR(x)))
#define CDDDR(x)  (CDR(CDDR(x)))
#define CADDDR(x)  (CAR(CDDDR(x)))
#define CDDDDR(x)  (CDR(CDDDR(x)))

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
    x_.output(state, o, inspect);
  }

protected:
  Svalue x_;
};

//=============================================================================

Vm* Vm::create(State* state) {
  void* memory = state->getAllocator()->alloc(sizeof(Vm));
  return new(memory) Vm(state);
}

void Vm::release() {
  Allocator* allocator = state_->getAllocator();
  this->~Vm();
  allocator->free(this);
}

Vm::~Vm() {
  if (stack_ != NULL)
    state_->getAllocator()->free(stack_);
  state_->getAllocator()->free(opcodes_);
}

Vm::Vm(State* state)
  : state_(state)
  , stack_(NULL), stackSize_(0)
  , stackPointer_(0), argNum_(0)
  , callStack_() {
  {
    void* memory = state_->getAllocator()->alloc(sizeof(Svalue) * NUMBER_OF_OPCODE);
    opcodes_ = new(memory) Svalue[NUMBER_OF_OPCODE];
    opcodes_[HALT] = state_->intern("HALT");
    opcodes_[UNDEF] = state_->intern("UNDEF");
    opcodes_[CONST] = state_->intern("CONST");
    opcodes_[LREF] = state_->intern("LREF");
    opcodes_[FREF] = state_->intern("FREF");
    opcodes_[GREF] = state_->intern("GREF");
    opcodes_[LSET] = state_->intern("LSET");
    opcodes_[FSET] = state_->intern("FSET");
    opcodes_[GSET] = state_->intern("GSET");
    opcodes_[PUSH] = state_->intern("PUSH");
    opcodes_[TEST] = state_->intern("TEST");
    opcodes_[CLOSE] = state_->intern("CLOSE");
    opcodes_[FRAME] = state_->intern("FRAME");
    opcodes_[APPLY] = state_->intern("APPLY");
    opcodes_[RET] = state_->intern("RET");
    opcodes_[SHIFT] = state_->intern("SHIFT");
    opcodes_[BOX] = state_->intern("BOX");
    opcodes_[UNBOX] = state_->intern("UNBOX");
    opcodes_[CONTI] = state_->intern("CONTI");
    opcodes_[NUATE] = state_->intern("NUATE");
    opcodes_[MACRO] = state_->intern("MACRO");
  }

  {
    void* memory = state_->getAllocator()->objAlloc(sizeof(SHashTable));
    globalVariableTable_ = new(memory) SHashTable(state_->getAllocator());
  }
}

void Vm::reportDebugInfo() const {
  std::cout << "Global variables:" << std::endl;
  std::cout << "  capacity: #" << globalVariableTable_->getCapacity() << std::endl;
  std::cout << "  entry:    #" << globalVariableTable_->getEntryCount() << std::endl;
  std::cout << "  conflict: #" << globalVariableTable_->getConflictCount() << std::endl;
  std::cout << "  maxdepth: #" << globalVariableTable_->getMaxDepth() << std::endl;
}

void Vm::assignNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum) {
  void* memory = state_->getAllocator()->objAlloc(sizeof(NativeFunc));
  NativeFunc* nativeFunc = new(memory) NativeFunc(func, minArgNum, maxArgNum);
  assignGlobal(state_->intern(name), Svalue(nativeFunc));
}

Svalue Vm::run(Svalue code) {
  Svalue nil = state_->nil();
  return run(nil, code, 0, nil, 0);
}

Svalue Vm::run(Svalue a, Svalue x, int f, Svalue c, int s) {
 again:
  //std::cout << "run: stack=" << s << ", x=" << x << std::endl;

  Svalue op = CAR(x);
  x = CDR(x);
  int opidx = findOpcode(op);
  switch (opidx) {
  case HALT:
    stackPointer_ = s;
    return a;
  case UNDEF:
    x = CAR(x);
    a = state_->nil();
    goto again;
  case CONST:
    a = CAR(x);
    x = CADR(x);
    goto again;
  case LREF:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      a = index(f, n);
    }
    goto again;
  case FREF:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      assert(c.getType() == TT_CLOSURE);
      a = static_cast<Closure*>(c.toObject())->getFreeVariable(n);
    }
    goto again;
  case GREF:
    {
      Svalue sym = CAR(x);
      x = CADR(x);
      assert(sym.getType() == TT_SYMBOL);
      bool exist;
      Svalue aa = referGlobal(sym, &exist);
      if (!exist) {
        sym.output(state_, std::cerr, true);
        std::cerr << ": ";
        state_->runtimeError("Unbound");
      }
      a = aa;
    }
    goto again;
  case LSET:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      Svalue box = index(f, n);
      assert(box.getType() == TT_BOX);
      static_cast<Box*>(box.toObject())->set(a);
    }
    goto again;
  case FSET:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      Svalue box = static_cast<Closure*>(c.toObject())->getFreeVariable(n);
      assert(box.getType() == TT_BOX);
      static_cast<Box*>(box.toObject())->set(a);
    }
    goto again;
  case GSET:
    {
      Svalue sym = CAR(x);
      x = CADR(x);
      assert(sym.getType() == TT_SYMBOL);
      assignGlobal(sym, a);
    }
    goto again;
  case PUSH:
    {
      x = CAR(x);
      s = push(a, s);
    }
    goto again;
  case TEST:
    {
      Svalue thn = CAR(x);
      Svalue els = CADR(x);
      x = state_->isTrue(a) ? thn : els;
    }
    goto again;
  case CLOSE:
    {
      Svalue nparam = CAR(x);
      int nfree = CADR(x).toFixnum();
      Svalue body = CADDR(x);
      x = CADDDR(x);
      int min = CAR(nparam).toFixnum();
      int max = CADR(nparam).toFixnum();
      a = createClosure(body, nfree, s, min, max);
      s -= nfree;
    }
    goto again;
  case FRAME:
    {
      Svalue ret = CAR(x);
      x = CADR(x);
      s = push(ret, push(state_->fixnumValue(f), push(c, s)));
    }
    goto again;
  case APPLY:
    {
      int argNum = CAR(x).toFixnum();
      if (!a.isObject() || !a.toObject()->isCallable()) {
        state_->runtimeError("Can't call");
      }

      pushCallStack(static_cast<Callable*>(a.toObject()));

      switch (a.getType()) {
      case TT_CLOSURE:
        {
          Closure* closure = static_cast<Closure*>(a.toObject());
          int min = closure->getMinArgNum(), max = closure->getMaxArgNum();
          if (argNum < min) {
            state_->runtimeError("Too few arguments");
          } else if (max >= 0 && argNum > max) {
            state_->runtimeError("Too many arguments");
          }

          int ds = 0;
          if (closure->hasRestParam())
            ds = modifyRestParams(argNum, min, s);
          s += ds;
          argNum += ds;
          x = closure->getBody();
          f = s;
          c = a;
          s = push(state_->fixnumValue(argNum), s);
        }
        break;
      case TT_NATIVEFUNC:
        {
          // Store current state in member variable for native function call.
          stackPointer_ = s;
          argNum_ = argNum;
          NativeFunc* native = static_cast<NativeFunc*>(a.toObject());
          a = native->call(state_, argNum);

          // do-return
          s -= argNum;
          x = index(s, 0);
          f = index(s, 1).toFixnum();
          c = index(s, 2);
          s -= 3;
          popCallStack();
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
      int argnum = index(s, 0).toFixnum();
      s -= argnum + 1;
      x = index(s, 0);
      f = index(s, 1).toFixnum();
      c = index(s, 2);
      s -= 3;
      popCallStack();
    }
    goto again;
  case SHIFT:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      int calleeArgnum = index(f, -1).toFixnum();
      s = shiftArgs(n, calleeArgnum, s);
      shiftCallStack();
    }
    goto again;
  case BOX:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      indexSet(f, n, box(index(f, n)));
    }
    goto again;
  case UNBOX:
    x = CAR(x);
    assert(a.getType() == TT_BOX);
    a = static_cast<Box*>(a.toObject())->get();
    goto again;
  case CONTI:
    x = CAR(x);
    a = createContinuation(s);
    goto again;
  case NUATE:
    {
      Svalue stack = CAR(x);
      x = CADR(x);
      s = restoreStack(stack);
    }
    goto again;
  case MACRO:
    {
      Svalue name = CAR(x);
      Svalue nparam = CADR(x);
      Svalue body = CADDR(x);
      x = CADDDR(x);
      int min = CAR(nparam).toFixnum();
      int max = CADR(nparam).toFixnum();
      Svalue closure = createClosure(body, 0, s, min, max);

      Svalue args[] = { name, closure };
      funcall(state_->referGlobal(state_->intern("register-macro")), sizeof(args) / sizeof(*args), args);
    }
    goto again;
  default:
    op.output(state_, std::cerr, true);
    std::cerr << ": ";
    state_->runtimeError("Unknown op");
    return a;
  }
}

int Vm::findOpcode(Svalue op) {
  for (int i = 0; i < NUMBER_OF_OPCODE; ++i)
    if (op.eq(opcodes_[i]))
      return i;
  return -1;
}

Svalue Vm::createClosure(Svalue body, int nfree, int s, int minArgNum, int maxArgNum) {
  void* memory = state_->getAllocator()->objAlloc(sizeof(Closure));
  Closure* closure = new(memory) Closure(state_, body, nfree, minArgNum, maxArgNum);
  for (int i = 0; i < nfree; ++i)
    closure->setFreeVariable(i, index(s, i));
  return Svalue(closure);
}

Svalue Vm::createContinuation(int s) {
  Svalue zero = state_->fixnumValue(0);
  Svalue body = list(state_,
                     opcodes_[LREF],
                     zero,
                     list(state_,
                          opcodes_[NUATE],
                          saveStack(s),
                          list(state_,
                               opcodes_[RET])));
  return createClosure(body, s, s, 0, 1);
}

Svalue Vm::box(Svalue x) {
  void* memory = state_->getAllocator()->objAlloc(sizeof(Box));
  return Svalue(new(memory) Box(x));
}

int Vm::push(Svalue x, int s) {
  if (s >= stackSize_)
    expandStack();
  stack_[s] = x;
  return s + 1;
}

void Vm::expandStack() {
  int newSize = stackSize_ + 16;
  void* memory = state_->getAllocator()->realloc(stack_, sizeof(Svalue) * newSize);
  Svalue* newStack = static_cast<Svalue*>(memory);
  if (newStack == NULL) {
    std::cerr << "Can't expand stack! size=" << newSize << std::endl;
    exit(1);
  }
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
  Svalue acc = state_->nil();
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
  return result != NULL ? *result : state_->nil();
}

void Vm::assignGlobal(Svalue sym, Svalue value) {
  if (sym.getType() != TT_SYMBOL)
    state_->runtimeError("Must be symbol");
  globalVariableTable_->put(sym, value);

  if (value.isObject() && value.toObject()->isCallable()) {
    Symbol* name = static_cast<Symbol*>(sym.toObject());
    static_cast<Callable*>(value.toObject())->setName(name);
  }
}

Svalue Vm::saveStack(int s) {
  Allocator* allocator = state_->getAllocator();
  void* memory = allocator->objAlloc(sizeof(Vector));
  Vector* v = new(memory) Vector(allocator, s);
  for (int i = 0; i < s; ++i) {
    v->set(i, stack_[i]);
  }
  return Svalue(v);
}

int Vm::restoreStack(Svalue v) {
  assert(v.getType() == TT_VECTOR);
  Vector* vv = static_cast<Vector*>(v.toObject());
  int s = vv->size();
  for (int i = 0; i < s; ++i) {
    stack_[i] = vv->get(i);
  }
  return s;
}

int Vm::getArgNum() const {
  return argNum_;
}

Svalue Vm::getArg(int index) const {
  return this->index(stackPointer_, index);
}

Svalue Vm::funcall(Svalue fn, int argNum, const Svalue* args) {
  if (!fn.isObject() || !fn.toObject()->isCallable()) {
    fn.output(state_, std::cerr, true);
    state_->runtimeError("Can't call");
    return state_->nil();
  }

  pushCallStack(static_cast<Callable*>(fn.toObject()));

  Svalue result;
  const int prevStackPointer = stackPointer_;
  const int prevArgNum = argNum_;

  switch (fn.getType()) {
  case TT_CLOSURE:
    {
      Svalue ret = state_->getConstant(State::SINGLE_HALT);
      Svalue c = state_->nil();
      int s = stackPointer_;
      // Makes frame.
      s = push(ret, push(state_->fixnumValue(s), push(c, s)));

      s = pushArgs(argNum, args, s);

      Closure* closure = static_cast<Closure*>(fn.toObject());
      int min = closure->getMinArgNum(), max = closure->getMaxArgNum();
      if (argNum < min) {
        state_->runtimeError("Too few arguments");
      } else if (max >= 0 && argNum > max) {
        state_->runtimeError("Too many arguments");
      }

      int ds = 0;
      if (closure->hasRestParam())
        ds = modifyRestParams(argNum, min, s);
      s += ds;
      argNum += ds;
      Svalue x = closure->getBody();
      int f = s;
      s = push(state_->fixnumValue(argNum), s);
      result = run(fn, x, f, fn, s);
    }
    break;
  case TT_NATIVEFUNC:
    {
      // No frame.
      int s = stackPointer_;
      s = pushArgs(argNum, args, s);

      // Store current state in member variable for native function call.
      stackPointer_ = s;
      argNum_ = argNum;
      NativeFunc* native = static_cast<NativeFunc*>(fn.toObject());
      result = native->call(state_, argNum);

      popCallStack();
    }
    break;
  default:
    assert(!"Must not happen");
    break;
  }

  stackPointer_ = prevStackPointer;
  argNum_ = prevArgNum;
  return result;
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
  assert(!callStack_.empty());
  size_t n = callStack_.size();
  if (n >= 2 && callStack_[n - 2].callable == callStack_[n - 1].callable &&
      callStack_[n - 2].isTailCall) {
    // Self tail recursive case: eliminate call stack.
    callStack_.pop_back();
  } else {
    callStack_[n - 1].isTailCall = true;
  }
}

}  // namespace yalp
