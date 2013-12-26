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

#define REPLACE_OPCODE

#ifdef __GNUC__
#define DIRECT_THREADED
#endif

#ifdef REPLACE_OPCODE
#define OPCVAL(op)  (Value(op))
#define OPIDX(op)  ((op).toFixnum())
#else
#define OPCVAL(op)  (opcodes_[op])
#define OPIDX(op)  (findOpcode(op))
#endif

#define FETCH_OP  (prex = x_, x_ = CDR(prex), OPIDX(CAR(prex)))

#ifdef DIRECT_THREADED
#define INIT_DISPATCH  NEXT;
#define CASE(opcode)  L_ ## opcode:
#define NEXT  goto *JumpTable[FETCH_OP];
#define END_DISPATCH

#else
#define INIT_DISPATCH  for (;;) { VMTRACE; switch (FETCH_OP) {
#define CASE(opcode) case opcode:
#define NEXT break
#define END_DISPATCH }}
#endif

#define VMTRACE                                  \
  if (trace_) {                                  \
    FileStream out(stdout);                      \
    std::cout << "run: stack=" << s_ << ", x=";  \
    x_.output(state_, &out, true);               \
    std::cout << std::endl;                      \
  }                                              \

namespace yalp {

//=============================================================================

#define OPS \
  OP(HALT) \
  OP(VOID) \
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
  OP(VALS) \
  OP(RECV) \
  OP(NIL) \

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
#define CADDDR(x)  (CAR(CDDDR(x)))
#define CDDDDR(x)  (CDR(CDDDR(x)))

//=============================================================================

static inline void moveStackElems(Value* stack, int dst, int src, int n) {
  if (n > 0)
    memmove(&stack[dst], &stack[src], sizeof(Value) * n);
}

static inline void moveValuesToStack(Value* stack, const Value* values, Value a, int n) {
  if (n <= 0)
    return;
  if (n > 0)
    memmove(stack, values, sizeof(Value) * (n - 1));
  stack[n - 1] = a;
}

static bool checkArgNum(State* state, Value fn, int argNum, int min, int max) {
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
  Box(Value x)
    : Sobject()
    , x_(x) {}
  virtual Type getType() const override  { return TT_BOX; }

  void set(Value x)  { x_ = x; }
  Value get()  { return x_; }

  virtual void output(State* state, Stream* o, bool inspect) const override {
    // This should not be output, but debug purpose.
    o->write("#<box ");
    x_.output(state, o, inspect);
    o->write('>');
  }

protected:
  Value x_;
};

//=============================================================================
// Inline methods

Value Vm::index(int s, int i) const {
  assert(s - i - 1 >= 0);
  return stack_[s - i - 1];
}

void Vm::indexSet(int s, int i, Value v) {
  assert(s - i - 1 >= 0);
  stack_[s - i - 1] = v;
}

int Vm::push(Value x, int s) {
  reserveStack(s + 1);
  stack_[s] = x;
  return s + 1;
}

int Vm::shiftArgs(int n, int m, int s) {
  moveStackElems(stack_, s - n - m - 1, s - n, n);
  return s - m - 1;
}

bool Vm::isTailCall(Value x) const  { return CAR(x).eq(OPCVAL(RET)); }
int Vm::pushCallFrame(Value ret, int s) {
  return push(ret, push(Value(f_), push(c_, s)));
}
int Vm::popCallFrame(int s) {
  x_ = index(s, 0);
  f_ = index(s, 1).toFixnum();
  c_ = index(s, 2);
  return s - 3;
}

int Vm::findOpcode(Value op) {
  for (int i = 0; i < NUMBER_OF_OPCODE; ++i)
    if (op.eq(opcodes_[i]))
      return i;
  return -1;
}

Value Vm::box(Value x) {
  void* memory = state_->objAlloc(sizeof(Box));
  return Value(new(memory) Box(x));
}

//=============================================================================

Vm* Vm::create(State* state) {
  void* memory = state->alloc(sizeof(Vm));
  return new(memory) Vm(state);
}

void Vm::release() {
  State* state = state_;
  this->~Vm();
  state->free(this);
}

Vm::~Vm() {
  if (values_ != NULL)
    state_->free(values_);
  if (stack_ != NULL)
    state_->free(stack_);
  state_->free(opcodes_);
}

Vm::Vm(State* state)
  : state_(state)
  , stack_(NULL), stackSize_(0)
  , trace_(false)
  , values_(NULL), valuesSize_(0), valueCount_(0)
  , callStack_() {
  {
    static const char* NameTable[NUMBER_OF_OPCODE] = {
#define OP(name)  #name,
      OPS
#undef OP
    };

    void* memory = state_->alloc(sizeof(Value) * NUMBER_OF_OPCODE);
    opcodes_ = new(memory) Value[NUMBER_OF_OPCODE];
    for (int i = 0; i < NUMBER_OF_OPCODE; ++i)
      opcodes_[i] = state_->intern(NameTable[i]);
  }

  {
    Value ht = state_->createHashTable();
    assert(ht.getType() == TT_HASH_TABLE);
    globalVariableTable_ = static_cast<SHashTable*>(ht.toObject());
  }

  {
    Value ht = state_->createHashTable();
    macroTable_ = static_cast<SHashTable*>(ht.toObject());
  }

  endOfCode_ = list(state_, OPCVAL(HALT));
  return_ = list(state_, OPCVAL(RET));

  a_ = c_ = Value::NIL;
  x_ = endOfCode_;
  f_ = s_ = 0;
}

void Vm::setTrace(bool b) {
  trace_ = b;
}

void Vm::resetError() {
  Value nil = Value::NIL;
  a_ = nil;
  x_ = endOfCode_;
  c_ = nil;
  f_ = s_ = 0;
  callStack_.clear();
}

void Vm::markRoot() {
  globalVariableTable_->mark();
  macroTable_->mark();
  // Mark stack.
  for (int n = s_, i = 0; i < n; ++i)
    stack_[i].mark();
  // Mark values.
  for (int n = valueCount_ - 1, i = 0; i < n; ++i)
    values_[i].mark();
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

Value Vm::referGlobal(Value sym, bool* pExist) {
  const Value* result = globalVariableTable_->get(sym);
  if (pExist != NULL)
    *pExist = result != NULL;
  return result != NULL ? *result : Value::NIL;
}

void Vm::defineGlobal(Value sym, Value value) {
  state_->checkType(sym, TT_SYMBOL);
  globalVariableTable_->put(sym, value);

  if (value.isObject() && value.toObject()->isCallable()) {
    const Symbol* name = sym.toSymbol(state_);
    static_cast<Callable*>(value.toObject())->setName(name);
  }
}

bool Vm::assignGlobal(Value sym, Value value) {
  state_->checkType(sym, TT_SYMBOL);
  if (globalVariableTable_->get(sym) == NULL)
    return false;

  globalVariableTable_->put(sym, value);
  return true;
}

Value Vm::getMacro(Value name) {
  const Value* result = macroTable_->get(name);
  return result != NULL ? *result : Value::NIL;
}

int Vm::pushArgs(int argNum, const Value* args, int s) {
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

Value Vm::funcall(Value fn, int argNum, const Value* args) {
  switch (fn.getType()) {
  case TT_CLOSURE:
  case TT_CONTINUATION:
    {
      Value oldX = x_;
      x_ = endOfCode_;
      funcallSetup(fn, argNum, args, false);
      Value result = runLoop();
      x_ = oldX;
      return result;
    }
  default:
    return funcallSetup(fn, argNum, args, false);
  }
}

Value Vm::funcallSetup(Value fn, int argNum, const Value* args, bool tailcall) {
  switch (fn.getType()) {
  case TT_CLOSURE:
    if (isTailCall(x_)) {
      // Shifts arguments.
      int calleeArgNum = index(f_, -1).toFixnum();
      s_ = pushArgs(argNum, args, s_);
      s_ = shiftArgs(argNum, calleeArgNum, s_);
      shiftCallStack();
    } else {
      // Makes frame.
      s_ = pushCallFrame(x_, s_);
      s_ = pushArgs(argNum, args, s_);
    }
    apply(fn, argNum);
    // runLoop will run after this function exited.
    return Value::NIL;
  case TT_NATIVEFUNC:
    {
      // Save to local variable, instead of creating call frame.
      int oldS = s_;
      Value oldX = x_;

      s_ = pushArgs(argNum, args, s_);
      apply(fn, argNum);

      if (tailcall)
        shiftCallStack();
      else
        popCallStack();

      s_ = oldS;
      x_ = oldX;
      return a_;
    }
  case TT_CONTINUATION:
    s_ = push(argNum == 0 ? Value::NIL : args[0], s_);
    apply(fn, argNum);
    // runLoop will run after this function exited.
    return Value::NIL;
  default:
    assert(!"Must not happen");
    return Value::NIL;
  }
}

void Vm::apply(Value fn, int argNum) {
  if (!fn.isObject() || !fn.toObject()->isCallable())
    state_->runtimeError("Can't call `%@`", &fn);

  valueCount_ = 1;
  switch (fn.getType()) {
  case TT_CLOSURE:
    {
      Closure* closure = static_cast<Closure*>(fn.toObject());
      int min = closure->getMinArgNum(), max = closure->getMaxArgNum();
      checkArgNum(state_, fn, argNum, min, max);
      pushCallStack(closure);

      int ds = 0;
      if (closure->hasRestParam())
        ds = modifyRestParams(argNum, min);
      argNum += ds;
      x_ = closure->getBody();
      f_ = s_;
      c_ = fn;
      s_ = push(Value(argNum), s_);
    }
    break;
  case TT_NATIVEFUNC:
    {
      NativeFunc* native = static_cast<NativeFunc*>(fn.toObject());
      int min = native->getMinArgNum(), max = native->getMaxArgNum();
      checkArgNum(state_, fn, argNum, min, max);
      pushCallStack(native);

      c_ = fn;
      f_ = s_;
      s_ = push(Value(argNum), s_);
      x_ = return_;
      a_ = native->call(state_);
      // x_ might be updated in the above call using #tailcall method.
    }
    break;
  case TT_CONTINUATION:
    {
      Continuation* continuation = static_cast<Continuation*>(fn.toObject());
      checkArgNum(state_, fn, argNum, 0, 1);
      pushCallStack(continuation);
      a_ = (argNum == 0) ? Value::NIL : index(s_, 0);

      int savedStackSize = continuation->getStackSize();
      const Value* savedStack = continuation->getStack();
      memcpy(stack_, savedStack, sizeof(Value) * savedStackSize);

      int callStackSize = continuation->getCallStackSize();
      if (callStackSize == 0)
        callStack_.clear();
      else {
        const CallStack* callStack = continuation->getCallStack();
        callStack_.resize(callStackSize);
        memcpy(&callStack_[0], callStack, sizeof(CallStack) * callStackSize);
      }
      s_ = popCallFrame(savedStackSize);
    }
    break;
  default:
    assert(!"Must not happen");
    break;
  }
}

Value Vm::createClosure(Value body, int nfree, int s, int minArgNum, int maxArgNum) {
  void* memory = state_->objAlloc(sizeof(Closure));
  Closure* closure = new(memory) Closure(state_, body, nfree, minArgNum, maxArgNum);
  for (int i = 0; i < nfree; ++i)
    closure->setFreeVariable(i, index(s, i));
  return Value(closure);
}

void Vm::defineMacro(Value name, Value func) {
  state_->checkType(name, TT_SYMBOL);
  if (!func.isObject() || !static_cast<Sobject*>(func.toObject())->isCallable())
    state_->runtimeError("`%@` is not callable", &func);
  static_cast<Callable*>(func.toObject())->setName(name.toSymbol(state_));
  macroTable_->put(name, func);
  defineGlobal(name, state_->intern("*macro*"));
}

Value Vm::createContinuation(int s) {
  void* memory = state_->objAlloc(sizeof(Continuation));
  return Value(new(memory) Continuation(state_, stack_, s,
                                        &callStack_[0], callStack_.size()));
}

void Vm::reserveStack(int n) {
  if (n <= stackSize_)
    return;

  int newSize = n + 16;
  void* memory = state_->realloc(stack_, sizeof(Value) * newSize);
  Value* newStack = static_cast<Value*>(memory);
  stack_ = newStack;
  stackSize_ = newSize;
}

int Vm::modifyRestParams(int argNum, int minArgNum) {
  Value rest = createRestParams(argNum, minArgNum, s_);
  int ds = 0;
  if (argNum <= minArgNum) {
    // No rest param space: move all args +1 to create space.
    ds = minArgNum - argNum + 1;  // This must be 1 if arguments count is checked, but just in case.
    reserveStack(s_ + ds);
    moveStackElems(stack_, s_ - argNum + ds, s_ - argNum, argNum);
  } else if (argNum > minArgNum + 1) {
    // Shrink stack for unused rest parameters.
    ds = (minArgNum + 1) - argNum;  // Negative value.
    moveStackElems(stack_, s_ - argNum + 1,  s_ - minArgNum, minArgNum);
  }
  s_ += ds;
  indexSet(s_, minArgNum, rest);
  return ds;
}

Value Vm::createRestParams(int argNum, int minArgNum, int s) {
  Value acc = Value::NIL;
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
    stack_[n] = Value(n);
    s_ += 1;
    f_ = n;
    return;
  }

  // Before: [z][y][x]f_[argnum][B][A]s_
  // After:  [z][y][x][B][A]f_[argnum+n]s_
  int argNum = stack_[f_].toFixnum();
  int src = f_, dst = src + n;
  reserveStack(s_ + n);
  stack_[f_] = Value(argNum + n);
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
  stack_[f_] = Value(argnum - n);
  int src = f_, dst = f_ - n;
  moveStackElems(stack_, dst, src, s_ - src);  // Shift stack.
  s_ -= n;
  f_ -= n;
}

void Vm::reserveValuesBuffer(int n) {
  if (n - 1 > valuesSize_) {
    // Allocation size is n - 1, because values[0] is stored in a_ register.
    void* memory = state_->realloc(values_, sizeof(Value) * (n - 1));
    Value* newBuffer = static_cast<Value*>(memory);
    values_ = newBuffer;
    valuesSize_ = n - 1;
  }
}

void Vm::storeValues(int n, int s) {
  reserveValuesBuffer(n);
  // [0] is already stored in a if n > 0.
  if (n == 0)
    a_ = Value::NIL;
  for (int i = 1; i < n; ++i)
    values_[n - i - 1] = index(s, i);  // Stores values in reverse order to fit stack structure.
  valueCount_ = n;
}

void Vm::restoreValues(int min, int max) {
  int argNum = valueCount_;
  checkArgNum(state_, Value::NIL, argNum, min, max);
  reserveStack(s_ + argNum);
  moveValuesToStack(&stack_[s_], values_, a_, argNum);
  s_ += argNum;
  int n = argNum;
  int ds = 0;
  if (max < 0) {  // Has rest params.
    ds = modifyRestParams(argNum, min);
    argNum += ds;
    n = min + 1;
  }
  expandFrame(n);
  //s_ -= argNum - n;
}

void Vm::replaceOpcodes(Value x) {
#ifdef REPLACE_OPCODE
  for (;;) {
    Value op = CAR(x);
    if (op.getType() != TT_SYMBOL) {
      assert(op.getType() == TT_FIXNUM);
      return;
    }
    int opidx = findOpcode(op);
    static_cast<Cell*>(x.toObject())->setCar(Value(opidx));
    x = CDR(x);
    switch (opidx) {
    case HALT: case APPLY: case RET:
      return;
    case VOID: case PUSH: case UNBOX: case NIL:
      break;
    case CONST: case LREF: case FREF: case GREF: case LSET: case FSET:
    case GSET: case DEF: case SHIFT: case BOX: case CONTI: case EXPND:
    case SHRNK: case VALS: case RECV:
      x = CDR(x);
      break;
    case TEST: case FRAME:
      replaceOpcodes(CAR(x));
      x = CDR(x);
      break;
    case CLOSE:
      replaceOpcodes(CADDR(x));
      x = CDDDR(x);
      break;
    case MACRO:
      replaceOpcodes(CADDDR(x));
      x = CDDDDR(x);
      break;
    default:
      state_->runtimeError("Unknown op `%@`", &op);
      return;
    }
  }
#else
  (void)x;
#endif
}

Value Vm::run(Value code) {
  replaceOpcodes(code);

  Value oldX = x_;
  a_ = c_ = Value::NIL;
  x_ = code;
  Value result = runLoop();

  x_ = oldX;
  return result;
}

Value Vm::runLoop() {
#ifdef DIRECT_THREADED
  static const void *JumpTable[] = {
#define OP(name)  &&L_ ## name ,
    OPS
#undef OP
  };
#endif

  Value prex;
  INIT_DISPATCH {
    CASE(HALT) {
      x_ = endOfCode_;
      return a_;
    }
    CASE(VOID) {
      a_ = Value::NIL;
      valueCount_ = 0;
    } NEXT;
    CASE(NIL) {
      a_ = Value::NIL;
    } NEXT;
    CASE(CONST) {
      a_ = CAR(x_);
      x_ = CDR(x_);
    } NEXT;
    CASE(LREF) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      a_ = index(f_, n);
    } NEXT;
    CASE(FREF) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      assert(c_.getType() == TT_CLOSURE);
      a_ = static_cast<Closure*>(c_.toObject())->getFreeVariable(n);
    } NEXT;
    CASE(GREF) {
      Value sym = CAR(x_);
      x_ = CDR(x_);
      assert(sym.getType() == TT_SYMBOL);
      bool exist;
      Value aa = referGlobal(sym, &exist);
      if (!exist)
        state_->runtimeError("Unbound `%@`", &sym);
      a_ = aa;
    } NEXT;
    CASE(LSET) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      Value box = index(f_, n);
      assert(box.getType() == TT_BOX);
      static_cast<Box*>(box.toObject())->set(a_);
    } NEXT;
    CASE(FSET) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      assert(c_.getType() == TT_CLOSURE);
      Value box = static_cast<Closure*>(c_.toObject())->getFreeVariable(n);
      assert(box.getType() == TT_BOX);
      static_cast<Box*>(box.toObject())->set(a_);
    } NEXT;
    CASE(GSET) {
      Value sym = CAR(x_);
      x_ = CDR(x_);
      assert(sym.getType() == TT_SYMBOL);
      if (!assignGlobal(sym, a_))
        state_->runtimeError("Global variable `%@` not defined", &sym);
    } NEXT;
    CASE(DEF) {
      Value sym = CAR(x_);
      x_ = CDR(x_);
      assert(sym.getType() == TT_SYMBOL);
      defineGlobal(sym, a_);
    } NEXT;
    CASE(PUSH) {
      s_ = push(a_, s_);
    } NEXT;
    CASE(TEST) {
      Value thn = CAR(x_);
      Value els = CDR(x_);
      x_ = a_.isTrue() ? thn : els;
    } NEXT;
    CASE(CLOSE) {
      Value nparam = CAR(x_);  // Fixnum (fixed parameters function) or Cell (arbitrary number of parameters function).
      int nfree = CADR(x_).toFixnum();
      Value body = CADDR(x_);
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
        CELL(prex)->setCar(OPCVAL(CONST));
        CELL(CDR(prex))->setCar(a_);
        CELL(CDR(prex))->setCdr(x_);
      }
    } NEXT;
    CASE(FRAME) {
      Value ret = CDR(x_);
      x_ = CAR(x_);
      s_ = pushCallFrame(ret, s_);
    } NEXT;
    CASE(APPLY) {
      int argNum = CAR(x_).toFixnum();
      apply(a_, argNum);
    } NEXT;
    CASE(RET) {
      int argNum = index(s_, 0).toFixnum();
      s_ = popCallFrame(s_ - argNum - 1);
      popCallStack();
    } NEXT;
    CASE(SHIFT) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      int calleeArgNum = index(f_, -1).toFixnum();
      s_ = shiftArgs(n, calleeArgNum, s_);
      shiftCallStack();
    } NEXT;
    CASE(BOX) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      indexSet(f_, n, box(index(f_, n)));
    } NEXT;
    CASE(UNBOX) {
      assert(a_.getType() == TT_BOX);
      a_ = static_cast<Box*>(a_.toObject())->get();
    } NEXT;
    CASE(CONTI) {
      Value tail = CAR(x_);
      x_ = CDR(x_);
      int s = s_;
      if (tail.isTrue()) {
        int calleeArgNum = index(s_, 0).toFixnum();
        s -= calleeArgNum + 1;
      }
      a_ = createContinuation(s);
    } NEXT;
    CASE(MACRO) {
      Value name = CAR(x_);
      Value nparam = CADR(x_);
      int nfree = CADDR(x_).toFixnum();
      Value body = CADDDR(x_);
      x_ = CDDDDR(x_);
      int min, max;
      if (nparam.getType() == TT_CELL) {
        min = CAR(nparam).toFixnum();
        max = CADR(nparam).toFixnum();
      } else {
        min = max = nparam.toFixnum();
      }
      defineMacro(name, createClosure(body, nfree, s_, min, max));
      s_ -= nfree;
    } NEXT;
    CASE(EXPND) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      expandFrame(n);
    } NEXT;
    CASE(SHRNK) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      shrinkFrame(n);
    } NEXT;
    CASE(VALS) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      storeValues(n, s_);
      s_ -= n;
    } NEXT;
    CASE(RECV) {
      Value nparam = CAR(x_);  // Fixnum (fixed parameters function) or Cell (arbitrary number of parameters function).
      x_ = CDR(x_);
      int min, max;
      if (nparam.getType() == TT_CELL) {
        min = CAR(nparam).toFixnum();
        max = CADR(nparam).toFixnum();
      } else {
        min = max = nparam.toFixnum();
      }
      restoreValues(min, max);
      valueCount_ = 1;
    } NEXT;
  } END_DISPATCH;
}

}  // namespace yalp
