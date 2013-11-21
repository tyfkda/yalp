//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#include "vm.hh"
#include "yalp/mem.hh"
#include "yalp/object.hh"
#include <assert.h>
#include <iostream>

namespace yalp {

//=============================================================================
// Native functions

static Svalue s_cons(State* state) {
  Svalue a = state->getArg(0);
  Svalue d = state->getArg(1);
  return state->cons(a, d);
}

static Svalue s_car(State* state) {
  Svalue cell = state->getArg(0);
  if (cell.getType() != TT_CELL) {
    state->runtimeError("Cell expected");
  }
  return static_cast<Cell*>(cell.toObject())->car();
}

static Svalue s_cdr(State* state) {
  Svalue cell = state->getArg(0);
  if (cell.getType() != TT_CELL) {
    state->runtimeError("Cell expected");
  }
  return static_cast<Cell*>(cell.toObject())->cdr();
}

static Svalue s_list(State* state) {
  int n = state->getArgNum();
  Svalue res = state->nil();
  for (int i = n; --i >= 0; ) {
    Svalue a = state->getArg(i);
    res = state->cons(a, res);
  }
  return res;
}

static Svalue s_listStar(State* state) {
  int n = state->getArgNum();
  Svalue res;
  if (n <= 0) {
    res = state->nil();
  } else {
    res = state->getArg(n - 1);
    for (int i = n - 1; --i >= 0; ) {
      Svalue a = state->getArg(i);
      res = state->cons(a, res);
    }
  }
  return res;
}

static Svalue s_consp(State* state) {
  Svalue a = state->getArg(0);
  return state->boolValue(a.getType() == TT_CELL);
}

static Svalue s_append(State* state) {
  int n = state->getArgNum();
  Svalue res;
  if (n <= 0) {
    res = state->nil();
  } else {
    res = state->getArg(n - 1);
    struct Local {
      static Svalue loop(State* state, Svalue a, Svalue d) {
        if (a.getType() != TT_CELL)
          return d;
        Cell* cell = static_cast<Cell*>(a.toObject());
        return state->cons(cell->car(), loop(state, cell->cdr(), d));
      }
    };
    for (int i = n - 1; --i >= 0; ) {
      Svalue a = state->getArg(i);
      res = Local::loop(state, a, res);
    }
  }
  return res;
}

static Svalue s_add(State* state) {
  int n = state->getArgNum();
  Sfixnum a = 0;
  for (int i = 0; i < n; ++i) {
    Svalue x = state->getArg(i);
    if (x.getType() != TT_FIXNUM) {
      state->runtimeError("Fixnum expected");
    }
    a += x.toFixnum();
  }
  return state->fixnumValue(a);
}

static Svalue s_sub(State* state) {
  int n = state->getArgNum();
  Sfixnum a;
  if (n <= 0) {
    a = 0;
  } else {
    Svalue x = state->getArg(0);
    if (x.getType() != TT_FIXNUM) {
      state->runtimeError("Fixnum expected");
    }
    a = x.toFixnum();
    if (n == 1) {
      a = -a;
    } else {
      for (int i = 1; i < n; ++i) {
        Svalue x = state->getArg(i);
        if (x.getType() != TT_FIXNUM) {
          state->runtimeError("Fixnum expected");
        }
        a -= x.toFixnum();
      }
    }
  }
  return state->fixnumValue(a);
}

static Svalue s_mul(State* state) {
  int n = state->getArgNum();
  Sfixnum a = 1;
  for (int i = 0; i < n; ++i) {
    Svalue x = state->getArg(i);
    if (x.getType() != TT_FIXNUM) {
      state->runtimeError("Fixnum expected");
    }
    a *= x.toFixnum();
  }
  return state->fixnumValue(a);
}

static Svalue s_div(State* state) {
  int n = state->getArgNum();
  Sfixnum a;
  if (n <= 0) {
    a = 1;
  } else {
    Svalue x = state->getArg(0);
    if (x.getType() != TT_FIXNUM) {
      state->runtimeError("Fixnum expected");
    }
    a = x.toFixnum();
    if (n == 1) {
      a = 1 / a;
    } else {
      for (int i = 1; i < n; ++i) {
        Svalue x = state->getArg(i);
        if (x.getType() != TT_FIXNUM) {
          state->runtimeError("Fixnum expected");
        }
        a /= x.toFixnum();
      }
    }
  }
  return state->fixnumValue(a);
}

static Svalue s_eq(State* state) {
  Svalue a = state->getArg(0);
  Svalue b = state->getArg(1);
  return state->boolValue(a.eq(b));
}

static Svalue s_equal(State* state) {
  Svalue a = state->getArg(0);
  Svalue b = state->getArg(1);
  return state->boolValue(a.equal(b));
}

template <class Comparator>
Svalue compare(State* state, Comparator c) {
  int n = state->getArgNum();
  bool b = true;
  if (n >= 1) {
    Svalue x = state->getArg(0);
    if (x.getType() != TT_FIXNUM) {
      state->runtimeError("Fixnum expected");
    }
    Sfixnum xx = x.toFixnum();
    for (int i = 1; i < n; ++i) {
      Svalue y = state->getArg(i);
      if (y.getType() != TT_FIXNUM) {
        state->runtimeError("Fixnum expected");
      }
      Sfixnum yy = y.toFixnum();
      if (!c.satisfy(xx, yy)) {
        b = false;
        break;
      }
      xx = yy;
    }
  }
  return state->boolValue(b);
}

static Svalue s_lessThan(State* state) {
  struct LessThan {
    static bool satisfy(Sfixnum x, Sfixnum y) { return x < y; }
  } comp;
  return compare(state, comp);
}

static Svalue s_greaterThan(State* state) {
  struct greaterThan {
    static bool satisfy(Sfixnum x, Sfixnum y) { return x > y; }
  } comp;
  return compare(state, comp);
}

static Svalue s_lessEqual(State* state) {
  struct LessThan {
    static bool satisfy(Sfixnum x, Sfixnum y) { return x <= y; }
  } comp;
  return compare(state, comp);
}

static Svalue s_greaterEqual(State* state) {
  struct greaterThan {
    static bool satisfy(Sfixnum x, Sfixnum y) { return x >= y; }
  } comp;
  return compare(state, comp);
}

static Svalue s_write(State* state) {
  Svalue x = state->getArg(0);
  std::cout << x;
  return state->nil();
}

static Svalue s_print(State* state) {
  s_write(state);
  std::cout << std::endl;
  return state->nil();
}

static Svalue s_newline(State* state) {
  std::cout << std::endl;
  return state->nil();
}

static Svalue s_uniq(State* state) {
  return state->gensym();
}

//=============================================================================

enum Opcode {
  HALT,
  UNDEF,
  REFER_LOCAL,
  REFER_FREE,
  REFER_GLOBAL,
  INDIRECT,
  CONSTANT,
  CLOSE,
  BOX,
  TEST,
  ASSIGN_LOCAL,
  ASSIGN_FREE,
  ASSIGN_GLOBAL,
  CONTI,
  NUATE,
  FRAME,
  ARGUMENT,
  SHIFT,
  APPLY,
  RETURN,

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

// Closure class.
class Closure : public Sobject {
public:
  Closure(State* state, Svalue body, int freeVarCount, int minArgNum, int maxArgNum)
    : Sobject()
    , body_(body)
    , freeVariables_(NULL)
    , minArgNum_(minArgNum)
    , maxArgNum_(maxArgNum) {
    if (freeVarCount > 0) {
      void* memory = state->getAllocator()->alloc(sizeof(Svalue) * freeVarCount);
      freeVariables_ = new(memory) Svalue[freeVarCount];
    }
  }
  virtual Type getType() const override  { return TT_CLOSURE; }

  Svalue getBody() const  { return body_; }
  int getMinArgNum() const  { return minArgNum_; }
  int getMaxArgNum() const  { return maxArgNum_; }
  bool hasRestParam() const  { return maxArgNum_ < 0; }

  void setFreeVariable(int index, Svalue value) {
    freeVariables_[index] = value;
  }

  Svalue getFreeVariable(int index) const {
    return freeVariables_[index];
  }

  virtual std::ostream& operator<<(std::ostream& o) const override {
    return o << "#<closure:" << this << ">";
  }

protected:
  Svalue body_;
  Svalue* freeVariables_;
  int minArgNum_;
  int maxArgNum_;
};

// Native function class.
class NativeFunc : public Sobject {
public:
  NativeFunc(NativeFuncType func, int minArgNum, int maxArgNum)
    : Sobject()
    , func_(func)
    , minArgNum_(minArgNum)
    , maxArgNum_(maxArgNum) {}
  virtual Type getType() const override  { return TT_NATIVEFUNC; }

  Svalue call(State* state, int argNum) {
    if (argNum < minArgNum_) {
      state->runtimeError("Too few arguments");
    } else if (maxArgNum_ >= 0 && argNum > maxArgNum_) {
      state->runtimeError("Too many arguments");
    }
    return func_(state);
  }

  virtual std::ostream& operator<<(std::ostream& o) const override {
    return o << "#<procedure:" << this << ">";
  }

protected:
  NativeFuncType func_;
  int minArgNum_;
  int maxArgNum_;
};

// Box class.
class Box : public Sobject {
public:
  Box(Svalue x)
    : Sobject()
    , x_(x) {}
  virtual Type getType() const override  { return TT_BOX; }

  void set(Svalue x)  { x_ = x; }
  Svalue get()  { return x_; }

  virtual std::ostream& operator<<(std::ostream& o) const override {
    return o << x_;
  }

protected:
  Svalue x_;
};

// Vector class.
class Vector : public Sobject {
public:
  Vector(int size)
    : Sobject()
    , size_(size) {
    buffer_ = new Svalue[size_];
  }
  virtual Type getType() const override  { return TT_VECTOR; }

  int size() const  { return size_; }

  Svalue get(int index)  {
    assert(0 <= index && index < size_);
    return buffer_[index];
  }

  void set(int index, Svalue x)  {
    assert(0 <= index && index < size_);
    buffer_[index] = x;
  }

  virtual std::ostream& operator<<(std::ostream& o) const override {
    o << "#";
    char c = '(';
    for (int i = 0; i < size_; ++i) {
      o << c << buffer_[i];
      c = ' ';
    }
    return o << ")";
  }

protected:
  Svalue* buffer_;
  int size_;
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
  free(stack_);
  state_->getAllocator()->free(opcodes_);
}

Vm::Vm(State* state)
  : state_(state)
  , stack_(NULL), stackSize_(0) {

  void* memory = state_->getAllocator()->alloc(sizeof(Svalue) * NUMBER_OF_OPCODE);
  opcodes_ = new(memory) Svalue[NUMBER_OF_OPCODE];
  opcodes_[HALT] = state_->intern("HALT");
  opcodes_[UNDEF] = state_->intern("UNDEF");
  opcodes_[REFER_LOCAL] = state_->intern("REFER-LOCAL");
  opcodes_[REFER_FREE] = state_->intern("REFER-FREE");
  opcodes_[REFER_GLOBAL] = state_->intern("REFER-GLOBAL");
  opcodes_[INDIRECT] = state_->intern("INDIRECT");
  opcodes_[CONSTANT] = state_->intern("CONSTANT");
  opcodes_[CLOSE] = state_->intern("CLOSE");
  opcodes_[BOX] = state_->intern("BOX");
  opcodes_[TEST] = state_->intern("TEST");
  opcodes_[ASSIGN_LOCAL] = state_->intern("ASSIGN-LOCAL");
  opcodes_[ASSIGN_FREE] = state_->intern("ASSIGN-FREE");
  opcodes_[ASSIGN_GLOBAL] = state_->intern("ASSIGN-GLOBAL");
  opcodes_[CONTI] = state_->intern("CONTI");
  opcodes_[NUATE] = state_->intern("NUATE");
  opcodes_[FRAME] = state_->intern("FRAME");
  opcodes_[ARGUMENT] = state_->intern("ARGUMENT");
  opcodes_[SHIFT] = state_->intern("SHIFT");
  opcodes_[APPLY] = state_->intern("APPLY");
  opcodes_[RETURN] = state_->intern("RETURN");

  installNativeFunctions();
}

void Vm::installNativeFunctions() {
  assignGlobal(state_->nil(), state_->nil());
  assignGlobal(state_->t(), state_->t());

  assignNative("cons", s_cons, 2);
  assignNative("car", s_car, 1);
  assignNative("cdr", s_cdr, 1);
  assignNative("list", s_list, 0, -1);
  assignNative("list*", s_listStar, 0, -1);
  assignNative("consp", s_consp, 1);
  assignNative("append", s_append, 0, -1);
  assignNative("+", s_add, 0, -1);
  assignNative("-", s_sub, 0, -1);
  assignNative("*", s_mul, 0, -1);
  assignNative("/", s_div, 0, -1);

  assignNative("eq", s_eq, 2);
  assignNative("equal", s_equal, 2);
  assignNative("<", s_lessThan, 2, -1);
  assignNative(">", s_greaterThan, 2, -1);
  assignNative("<=", s_lessEqual, 2, -1);
  assignNative(">=", s_greaterEqual, 2, -1);

  assignNative("print", s_print, 1);
  assignNative("display", s_write, 1);
  assignNative("write", s_write, 1);
  assignNative("newline", s_newline, 0);

  assignNative("uniq", s_uniq, 0);
}

void Vm::assignNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum) {
  void* memory = state_->getAllocator()->alloc(sizeof(NativeFunc));
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
    return a;
  case UNDEF:
    x = CAR(x);
    a = state_->nil();
    goto again;
  case CONSTANT:
    a = CAR(x);
    x = CADR(x);
    goto again;
  case INDIRECT:
    x = CAR(x);
    assert(a.getType() == TT_BOX);
    a = static_cast<Box*>(a.toObject())->get();
    goto again;
  case REFER_LOCAL:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      a = index(f, n);
    }
    goto again;
  case REFER_FREE:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      assert(c.getType() == TT_CLOSURE);
      a = static_cast<Closure*>(c.toObject())->getFreeVariable(n);
    }
    goto again;
  case REFER_GLOBAL:
    {
      Svalue sym = CAR(x);
      x = CADR(x);
      assert(sym.getType() == TT_SYMBOL);
      if (!referGlobal(sym, &a)) {
        std::cerr << sym << ": ";
        state_->runtimeError("Unbound");
      }
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
  case BOX:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      indexSet(f, n, box(index(f, n)));
    }
    goto again;
  case TEST:
    {
      Svalue thn = CAR(x);
      Svalue els = CADR(x);
      x = state_->isTrue(a) ? thn : els;
    }
    goto again;
  case ASSIGN_LOCAL:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      Svalue box = index(f, n);
      assert(box.getType() == TT_BOX);
      static_cast<Box*>(box.toObject())->set(a);
    }
    goto again;
  case ASSIGN_FREE:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      Svalue box = static_cast<Closure*>(c.toObject())->getFreeVariable(n);
      assert(box.getType() == TT_BOX);
      static_cast<Box*>(box.toObject())->set(a);
    }
    goto again;
  case ASSIGN_GLOBAL:
    {
      Svalue sym = CAR(x);
      x = CADR(x);
      assert(sym.getType() == TT_SYMBOL);
      assignGlobal(sym, a);
    }
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
  case FRAME:
    {
      Svalue ret = CAR(x);
      x = CADR(x);
      s = push(ret, push(state_->fixnumValue(f), push(c, s)));
    }
    goto again;
  case SHIFT:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      int calleeArgnum = index(f, -1).toFixnum();
      s = shiftArgs(n, calleeArgnum, s);
    }
    goto again;
  case ARGUMENT:
    {
      x = CAR(x);
      s = push(a, s);
    }
    goto again;
  case APPLY:
    {
      int argNum = CAR(x).toFixnum();
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
        }
        break;
      default:
        state_->runtimeError("Can't call");
        break;
      }
    }
    goto again;
  case RETURN:
    {
      int argnum = index(s, 0).toFixnum();
      s -= argnum + 1;
      x = index(s, 0);
      f = index(s, 1).toFixnum();
      c = index(s, 2);
      s -= 3;
    }
    goto again;
  default:
    std::cerr << op << ": ";
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
  void* memory = state_->getAllocator()->alloc(sizeof(Closure));
  Closure* closure = new(memory) Closure(state_, body, nfree, minArgNum, maxArgNum);
  for (int i = 0; i < nfree; ++i)
    closure->setFreeVariable(i, index(s, i));
  return Svalue(closure);
}

Svalue Vm::createContinuation(int s) {
  Svalue zero = state_->fixnumValue(0);
  Svalue body = list(state_,
                     opcodes_[REFER_LOCAL],
                     zero,
                     list(state_,
                          opcodes_[NUATE],
                          saveStack(s),
                          list(state_,
                               opcodes_[RETURN])));
  return createClosure(body, s, s, 0, 1);
}

Svalue Vm::box(Svalue x) {
  void* memory = state_->getAllocator()->alloc(sizeof(Box));
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

bool Vm::referGlobal(Svalue sym, Svalue* pValue) {
  auto it = globalVariableTable_.find(sym.getHash());
  if (it == globalVariableTable_.end())
    return false;
  *pValue = it->second;
  return true;
}

void Vm::assignGlobal(Svalue sym, Svalue value) {
  globalVariableTable_[sym.getHash()] = value;
}

Svalue Vm::saveStack(int s) {
  Vector* v = new Vector(s);
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

}  // namespace yalp
