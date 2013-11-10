//=============================================================================
/// Vm - Macro Processor VM.
//=============================================================================

#include "vm.hh"
#include "macp.hh"
#include <assert.h>
#include <iostream>

namespace macp {

enum Opcode {
  HALT,
  REFER_LOCAL,
  REFER_FREE,
  CONSTANT,
  CLOSE,
  BOX,
  TEST,
  ASSIGN_LOCAL,
  ASSIGN_FREE,
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
  Closure(Svalue body, int freeVarCount)
    : Sobject()
    , body_(body)
    , freeVariables_(NULL) {
    if (freeVarCount > 0)
      freeVariables_ = new Svalue[freeVarCount];
  }
  virtual Type getType() const override  { return TT_CLOSURE; }

  Svalue getBody() const  { return body_; }

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
};

// Box class.
class Box : public Sobject {
public:
  Box(Svalue x)
    : Sobject()
    , x_(x) {}
  virtual Type getType() const override  { return TT_BOX; }

  void set(Svalue x)  { x_ = x; }

  virtual std::ostream& operator<<(std::ostream& o) const override {
    return o << x_;
  }

protected:
  Svalue x_;
};

Vm* Vm::create(State* state) {
  return new Vm(state);
}

Vm::~Vm() {
  free(stack_);
  delete[] opcodes_;
}

Vm::Vm(State* state)
  : state_(state)
  , stack_(NULL), stackSize_(0) {

  opcodes_ = new Svalue[NUMBER_OF_OPCODE];
  opcodes_[HALT] = state_->intern("HALT");
  opcodes_[REFER_LOCAL] = state_->intern("REFER-LOCAL");
  opcodes_[REFER_FREE] = state_->intern("REFER-FREE");
  opcodes_[CONSTANT] = state_->intern("CONSTANT");
  opcodes_[CLOSE] = state_->intern("CLOSE");
  opcodes_[BOX] = state_->intern("BOX");
  opcodes_[TEST] = state_->intern("TEST");
  opcodes_[ASSIGN_LOCAL] = state_->intern("ASSIGN-LOCAL");
  opcodes_[ASSIGN_FREE] = state_->intern("ASSIGN-FREE");
  opcodes_[FRAME] = state_->intern("FRAME");
  opcodes_[ARGUMENT] = state_->intern("ARGUMENT");
  opcodes_[SHIFT] = state_->intern("SHIFT");
  opcodes_[APPLY] = state_->intern("APPLY");
  opcodes_[RETURN] = state_->intern("RETURN");
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
  if (opidx < 0) {
    std::cerr << "Unknown op: " << op << std::endl;
    return op;
  }

  switch (opidx) {
  case HALT:
    break;
  case CONSTANT:
    a = CAR(x);
    x = CADR(x);
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
  case CLOSE:
    {
      int n = CAR(x).toFixnum();
      Svalue body = CADR(x);
      x = CADDR(x);
      a = createClosure(body, n, s);
      s -= n;
    }
    goto again;
  case BOX:
    {
      int n = CAR(x).toFixnum();
      x = CADR(x);
      indexSet(s, n, box(index(s, n)));
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
      int m = CADR(x).toFixnum();
      x = CADDR(x);
      s = shiftArgs(n, m, s);
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
      if (a.getType() == TT_CLOSURE) {
        x = static_cast<Closure*>(a.toObject())->getBody();
        f = s;
        c = a;
        goto again;
      }

      std::cerr << "Can't call " << a << std::endl;
    }
    goto again;
  case RETURN:
    {
      int n = CAR(x).toFixnum();
      s -= n;
      x = index(s, 0);
      f = index(s, 1).toFixnum();
      c = index(s, 2);
      s -= 3;
    }
    goto again;
  default:
    std::cerr << "Unknown op: " << op << std::endl;
    assert(false);
    break;
  }
  return a;
}

int Vm::findOpcode(Svalue op) {
  for (int i = 0; i < NUMBER_OF_OPCODE; ++i)
    if (op.eq(opcodes_[i]))
      return i;
  return -1;
}

Svalue Vm::createClosure(Svalue body, int n, int s) {
  Closure* closure = new Closure(body, n);
  for (int i = 0; i < n; ++i) {
    closure->setFreeVariable(i, index(s, i));
  }
  return Svalue(closure);
}

Svalue Vm::box(Svalue x) {
  return new Box(x);
}

int Vm::push(Svalue x, int s) {
  if (s >= stackSize_)
    expandStack();
  stack_[s] = x;
  return s + 1;
}

void Vm::expandStack() {
  int newSize = stackSize_ + 16;
  Svalue* newStack = static_cast<Svalue*>(realloc(stack_, sizeof(Svalue) * newSize));
  if (newStack == NULL) {
    std::cerr << "Can't expand stack! size=" << newSize << std::endl;
    exit(1);
  }
  stack_ = newStack;
  stackSize_ = newSize;
}

Svalue Vm::index(int s, int i) {
  assert(s - i - 1 >= 0);
  return stack_[s - i - 1];
}

void Vm::indexSet(int s, int i, Svalue v) {
  assert(s - i - 1 >= 0);
  stack_[s - i - 1] = v;
}

int Vm::shiftArgs(int n, int m, int s) {
  for (int i = n; --i >= 0; ) {
    indexSet(s, i + m, index(s, i));
  }
  return s - m;
}

}  // namespace macp
