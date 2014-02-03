//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#include "vm.hh"
#include "allocator.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"
#include "symbol_manager.hh"

#ifndef DISABLE_FLONUM
#include "flonum.hh"
#endif

#include <assert.h>
#include <iostream>
#include <string.h>  // for memmove

#ifdef __GNUC__
#define DIRECT_THREADED
#endif

#define OPCVAL(op)  (Value(op))

#define FETCH_OP  (prex = x_, x_ = CDR(prex), CAR(prex).toFixnum())

#ifdef DIRECT_THREADED
#define INIT_DISPATCH  NEXT;
#define CASE(opcode)  L_ ## opcode:
#define NEXT  goto *JumpTable[FETCH_OP];
#define END_DISPATCH
#define OTHERWISE  L_otherwise:

#else
#define INIT_DISPATCH  for (;;) { VMTRACE; switch (FETCH_OP) {
#define CASE(opcode) case opcode:
#define NEXT break
#define END_DISPATCH }}
#define OTHERWISE default:
#endif

#define VMTRACE                                                         \
  if (trace_) {                                                         \
    FileStream out(stdout);                                             \
    Value op = car(x_), d = car(cdr(x_));                               \
    if (d.getType() == TT_CELL) d = Value::NIL;                         \
    format(state_, &out, "run: s=%d, f=%d, x=%s %@\n", s_, f_, OpcodeNameTable[op.toFixnum()], &d); \
  }                                                                     \

namespace yalp {

//=============================================================================

enum Opcode {
#define OP(name)  name,
# include "opcodes.hh"
#undef OP
  NUMBER_OF_OPCODE
};

#ifndef DIRECT_THREADED
static const char* OpcodeNameTable[NUMBER_OF_OPCODE] = {
#define OP(name)  #name,
# include "opcodes.hh"
#undef OP
};
#endif

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
class Box : public Object {
public:
  Box(Value x)
    : Object()
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

#ifdef DISABLE_FLONUM
extern Value s_add(State*);
extern Value s_sub(State*);
extern Value s_mul(State*);
extern Value s_div(State*);
extern Value s_lessThan(State*);
extern Value s_lessEqual(State*);
extern Value s_greaterThan(State*);
extern Value s_greaterEqual(State*);
#else
#define s_add  s_addFlonum
#define s_sub  s_subFlonum
#define s_mul  s_mulFlonum
#define s_div  s_divFlonum
#define s_lessThan  s_lessThanFlonum
#define s_lessEqual  s_lessEqualFlonum
#define s_greaterThan  s_greaterThanFlonum
#define s_greaterEqual  s_greaterEqualFlonum
#endif

#define SIMPLE_EMBED_INST(OP, func)             \
  CASE(OP) {                                    \
    Fixnum n = CAR(x_).toFixnum();              \
    x_ = CDR(x_);                               \
    a_ = callEmbedFunction(func, n);            \
  } NEXT

template <class Op>
struct UnaryOp {
  static Value calc(State* state, Value a) {
    switch (a.getType()) {
    case TT_FIXNUM:
      return Value(Op::calc(a.toFixnum()));
#ifndef DISABLE_FLONUM
    case TT_FLONUM:
      return state->flonum(Op::calc(a.toFlonum(state)));
#endif
    default:
      state->runtimeError("Number expected, but `%@`", &a);
      return a;
    }
  }
};

struct Neg {
  template <class X> static X calc(X x)  { return -x; }
};
struct Inv {
  template <class X> static X calc(X x)  { return 1 / x; }
};

//=============================================================================
// Inline methods

Value Vm::callEmbedFunction(NativeFuncType func, Fixnum n) {
  int oldF = f_;
  f_ = s_;
  s_ = push(Value(n), s_);
  Value a = (*func)(state_);
  s_ -= n + 1;
  f_ = oldF;
  return a;
}

int Vm::push(Value x, int s) {
  reserveStack(s + 1);
  stack_[s] = x;
  return s + 1;
}

int Vm::shiftArgs(int n, int m, int s, int f) {
  moveStackElems(stack_, f - m, s - n, n);
  return f - m + n;
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

Value Vm::box(Value x) {
  return Value(state_->getAllocator()->newObject<Box>(x));
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
}

Vm::Vm(State* state)
  : state_(state)
  , stack_(NULL), stackSize_(0)
  , trace_(false)
  , values_(NULL), valuesSize_(0), valueCount_(0)
  , callStack_() {
  int arena = state_->saveArena();

  globalVariableTable_ = state_->createHashTable(false);

  endOfCode_ = list(state_, OPCVAL(HALT));
  return_ = list(state_, OPCVAL(RET));

  a_ = c_ = Value::NIL;
  x_ = endOfCode_;
  f_ = s_ = 0;
  state_->restoreArena(arena);
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
  Value v = referGlobal(name, NULL);
  if (v.getType() != TT_MACRO)
    return Value::NIL;
  return v;
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
  case TT_MACRO:
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
  case TT_MACRO:
    if (isTailCall(x_)) {
      // Shifts arguments.
      int calleeArgNum = index(f_, -1).toFixnum();
      s_ = pushArgs(argNum, args, s_);
      s_ = shiftArgs(argNum, calleeArgNum, s_, f_);
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
      int oldF = f_;
      Value oldX = x_;

      s_ = pushArgs(argNum, args, s_);
      apply(fn, argNum);

      if (tailcall)
        shiftCallStack();
      else
        popCallStack();

      s_ = oldS;
      f_ = oldF;
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
  case TT_MACRO:
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

      int arena = state_->saveArena();
      c_ = fn;
      f_ = s_;
      s_ = push(Value(argNum), s_);
      x_ = return_;
      a_ = native->call(state_);
      // x_ might be updated in the above call using #tailcall method.
      state_->restoreArena(arena);
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

static Value applyFunctionNormal(State* state, Vm* vm) {
  int n = vm->getArgNum();
  // Counts argument number for the given function.
  int argNum = n - 1;
  Value last = Value::NIL;
  if (n > 1) {
    // Last argument should be a list and its elements are function arguments.
    last = vm->getArg(n - 1);
    if (last.eq(Value::NIL))
      argNum -= 1;
    else {
      state->checkType(last, TT_CELL);
      argNum += length(last) - 1;
    }
  }

  Value* args = NULL;
  if (argNum > 0)
    args = static_cast<Value*>(alloca(sizeof(Value*) * argNum));
  for (int i = 0; i < argNum; ++i) {
    if (i < n - 2)
      args[i] = vm->getArg(i + 1);
    else {
      args[i] = car(last);
      last = cdr(last);
    }
  }

  Value fn = vm->getArg(0);
  return vm->tailcall(fn, argNum, args);
}

Value Vm::applyFunction() {
  Value f = getArg(0);
  switch (f.getType()) {
  case TT_CLOSURE:
  case TT_MACRO:
    return applyFunctionClosure();
  default:
    return applyFunctionNormal(state_, this);
  }
}

// Apply closure: Avoid list creation for closure which has rest parameter.
Value Vm::applyFunctionClosure() {
  State* state = state_;
  Vm* vm = this;
  Value fn = vm->getArg(0);
  Closure* closure = static_cast<Closure*>(fn.toObject());
  if (!closure->hasRestParam())
    return applyFunctionNormal(state, vm);

  int n = vm->getArgNum();
  int argNum = closure->getMinArgNum() + 1;
  Value* args = static_cast<Value*>(alloca(sizeof(Value*) * argNum));
  int idx = 1;
  Value ls = vm->getArg(n - 1);

  for (int i = 0; i < argNum - 1; ++i) {
    if (idx < n - 1) {
      args[i] = vm->getArg(idx++);
    } else if (ls.eq(Value::NIL)) {
      state->runtimeError("Too few arguments for %@", &fn);
    } else {
      args[i] = car(ls);
      ls = cdr(ls);
    }
  }
  if (idx < n - 1) {
    for (int i = n - 1; --i >= idx; )
      ls = state->cons(vm->getArg(i), ls);
  }
  args[argNum - 1] = ls;

  if (isTailCall(x_)) {
    // Shifts arguments.
    int calleeArgNum = index(f_, -1).toFixnum();
    s_ = pushArgs(argNum, args, s_);
    s_ = shiftArgs(argNum, calleeArgNum, s_, f_);
    shiftCallStack();
  } else {
    // Makes frame.
    s_ = pushCallFrame(x_, s_);
    s_ = pushArgs(argNum, args, s_);
  }

  pushCallStack(closure);
  x_ = closure->getBody();
  f_ = s_;
  c_ = fn;
  s_ = push(Value(argNum), s_);

  // runLoop will run after this function exited.
  return Value::NIL;
}

Value Vm::createClosure(Value body, int nfree, int s, int minArgNum, int maxArgNum) {
  Closure* closure = state_->getAllocator()->newObject<Closure>(state_, body, nfree, minArgNum, maxArgNum);
  for (int i = 0; i < nfree; ++i)
    closure->setFreeVariable(i, index(s, i));
  return Value(closure);
}

void Vm::defineMacro(Value name, Value body, int nfree, int s,
                     int minArgNum, int maxArgNum) {
  state_->checkType(name, TT_SYMBOL);
  Macro* macro = state_->getAllocator()->newObject<Macro>(state_, name, body, nfree,
                                                          minArgNum, maxArgNum);
  for (int i = 0; i < nfree; ++i)
    macro->setFreeVariable(i, index(s, i));
  defineGlobal(name, Value(macro));
}

Value Vm::createContinuation(int s) {
  Continuation* cont = state_->getAllocator()->newObject<Continuation>(
      state_, stack_, s, &callStack_[0], callStack_.size());
  return Value(cont);
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
  int arena = state_->saveArena();
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
  state_->restoreArena(arena);
  return ds;
}

Value Vm::createRestParams(int argNum, int minArgNum, int s) {
  Value acc = Value::NIL;
  for (int i = argNum; --i >= minArgNum; )
    acc = state_->cons(index(s, i), acc);
  return acc;
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

int Vm::restoreValues(int min, int max) {
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
  return n;
}

Value Vm::run(Value code) {
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
# include "opcodes.hh"
#undef OP
    &&L_otherwise
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
      valueCount_ = 1;
    } NEXT;
    CASE(FREF) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      assert(c_.getType() == TT_CLOSURE || c_.getType() == TT_MACRO);
      a_ = static_cast<Closure*>(c_.toObject())->getFreeVariable(n);
      valueCount_ = 1;
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
      valueCount_ = 1;
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
      assert(c_.getType() == TT_CLOSURE || c_.getType() == TT_MACRO);
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
      int arena = state_->saveArena();
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
      state_->restoreArena(arena);
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
      int argNum = index(f_, -1).toFixnum();
      s_ = popCallFrame(f_ - argNum);
      popCallStack();
    } NEXT;
    CASE(UNFRAME) {
      s_ = popCallFrame(s_);
    } NEXT;
    CASE(LOOP) {
      // Tail self recursive call (goto): Like SHIFT.
      int offset = CAR(x_).toFixnum();
      int n = CADR(x_).toFixnum();
      x_ = CDDR(x_);
      // TODO: Remove this conditional.
      if (offset > 0) {
        for (int i = 0; i < n; ++i)
          indexSet(f_, -(offset + i) - 2, index(s_, i));
      } else {
        for (int i = 0; i < n; ++i)
          indexSet(f_, offset + i, index(s_, i));
      }
      s_ -= n;
    } NEXT;
    CASE(TAPPLY) {
      // SHIFT
      int n = CAR(x_).toFixnum();
      int calleeArgNum = index(f_, -1).toFixnum();
      s_ = shiftArgs(n, calleeArgNum, s_, f_);
      shiftCallStack();
      // APPLY
      apply(a_, n);
    } NEXT;
    CASE(BOX) {
      int arena = state_->saveArena();
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      indexSet(f_, n, box(index(f_, n)));
      state_->restoreArena(arena);
    } NEXT;
    CASE(UNBOX) {
      assert(a_.getType() == TT_BOX);
      a_ = static_cast<Box*>(a_.toObject())->get();
    } NEXT;
    CASE(CONTI) {
      int arena = state_->saveArena();
      Value tail = CAR(x_);
      x_ = CDR(x_);
      int s = s_;
      if (tail.isTrue()) {
        int calleeArgNum = index(f_, -1).toFixnum();
        s = f_ - calleeArgNum;
      }
      a_ = createContinuation(s);
      state_->restoreArena(arena);
    } NEXT;
    CASE(SETJMP) {
      // Setjmp stores current closure and stack/frame pointer onto stack.
      int offset = CAR(x_).toFixnum();
      Value ret = CDDR(x_);
      x_ = CADR(x_);
      // Encode frame pointer and stack pointer value into single integer
      // for setjmp continuation.
      const int shiftBits = sizeof(Fixnum) / 2 * 8;
      Fixnum v = s_ | (static_cast<Fixnum>(f_) << shiftBits);
      indexSet(f_, -offset - 2, Value(f_ + offset + 1));  // Pointer.
      indexSet(f_, -offset - 3, Value(v));
      indexSet(f_, -offset - 4, c_);
      indexSet(f_, -offset - 5, ret);
    } NEXT;
    CASE(LONGJMP) {
      int p = a_.toFixnum();
      a_ = index(s_, 0);
      const int shiftBits = sizeof(Fixnum) / 2 * 8;
      const Fixnum mask = (static_cast<Fixnum>(1) << shiftBits) - 1;
      Fixnum v = stack_[p + 1].toFixnum();
      c_ = stack_[p + 2];
      x_ = stack_[p + 3];
      s_ = v & mask;
      f_ = v >> shiftBits;
    } NEXT;
    CASE(MACRO) {
      int arena = state_->saveArena();
      Value name = CAR(x_);
      Value nparam = CADR(x_);
      int nfree = CADDR(x_).toFixnum();
      Value tmp = CDDDR(x_);
      Value body = CAR(tmp);
      x_ = CDR(tmp);
      int min, max;
      if (nparam.getType() == TT_CELL) {
        min = CAR(nparam).toFixnum();
        max = CADR(nparam).toFixnum();
      } else {
        min = max = nparam.toFixnum();
      }
      defineMacro(name, body, nfree, s_, min, max);
      s_ -= nfree;
      state_->restoreArena(arena);
      valueCount_ = 0;
    } NEXT;
    CASE(ADDSP) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      s_ += n;
      reserveStack(s_);
    } NEXT;
    CASE(LOCAL) {
      int offset = CAR(x_).toFixnum();
      x_ = CDR(x_);
      indexSet(f_, offset, a_);
    } NEXT;
    CASE(VALS) {
      int n = CAR(x_).toFixnum();
      x_ = CDR(x_);
      storeValues(n, s_);
      s_ -= n;
    } NEXT;
    CASE(RECV) {
      int offset = CAR(x_).toFixnum();
      Value nparam = CADR(x_);  // Fixnum (fixed parameters function) or Cell (arbitrary number of parameters function).
      x_ = CDDR(x_);
      int min, max;
      if (nparam.getType() == TT_CELL) {
        min = CAR(nparam).toFixnum();
        max = CADR(nparam).toFixnum();
      } else {
        min = max = nparam.toFixnum();
      }
      int n = restoreValues(min, max);
      for (int i = 0; i < n; ++i)
        indexSet(f_, offset - i, index(s_, i));
      s_ -= n;
      valueCount_ = 1;
    } NEXT;
    CASE(CAR) {
      a_ = car(a_);
    } NEXT;
    CASE(CDR) {
      a_ = cdr(a_);
    } NEXT;
    SIMPLE_EMBED_INST(ADD, s_add);
    SIMPLE_EMBED_INST(SUB, s_sub);
    SIMPLE_EMBED_INST(MUL, s_mul);
    SIMPLE_EMBED_INST(DIV, s_div);
    SIMPLE_EMBED_INST(LT, s_lessThan);
    SIMPLE_EMBED_INST(LE, s_lessEqual);
    SIMPLE_EMBED_INST(GT, s_greaterThan);
    SIMPLE_EMBED_INST(GE, s_greaterEqual);
    CASE(EQ) {
      Value b = index(s_, 0);
      a_ = state_->boolean(a_.eq(b));
      --s_;
    } NEXT;
    CASE(NEG) {
      a_ = UnaryOp<Neg>::calc(state_, a_);
    } NEXT;
    CASE(INV) {
      a_ = UnaryOp<Inv>::calc(state_, a_);
    } NEXT;
    OTHERWISE {
      Value op = car(prex);
      state_->runtimeError("Unknown op `%@`", &op);
    } NEXT;
  } END_DISPATCH;
}

}  // namespace yalp
