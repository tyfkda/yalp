//=============================================================================
/// Read S-Expression
//=============================================================================

#include "build_env.hh"

#include "yalp/binder.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"
#include "allocator.hh"

#ifdef _MSC_VER
#include <math.h>
#else
#include <alloca.h>
#include <tgmath.h>
#endif

namespace yalp {

#ifndef DISABLE_FLONUM
// !!!
Value State::flonum(Flonum f) {
  return Value(allocator_->newObject<SFlonum>(f));
}

//=============================================================================

SFlonum::SFlonum(Flonum v)
  : Object()
  , v_(v) {
}

Type SFlonum::getType() const  { return TT_FLONUM; }

bool SFlonum::equal(const Object* target) const {
  const SFlonum* p = static_cast<const SFlonum*>(target);
  return v_ == p->v_;
}

void SFlonum::output(State*, Stream* o, bool) const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%f", v_);
  o->write(buffer);
}

//=============================================================================

namespace {

template <class Op>
struct BinOp {
  static Value calc(State* state) {
    int n = state->getArgNum();
    if (n <= 0)
      return Value(Op::base());
    Value x = state->getArg(0);
    Fixnum acc;
    switch (x.getType()) {
    case TT_FIXNUM:
      acc = x.toFixnum();
      break;
    case TT_FLONUM:
      return calcf(state, 1, x.toFlonum(state));
      break;
    default:
      state->runtimeError("Number expected, but `%@`", &x);
      break;
    }
    if (n == 1)
      return Value(Op::single(acc));

    for (int i = 1; i < n; ++i) {
      Value x = state->getArg(i);
      switch (x.getType()) {
      case TT_FIXNUM:
        acc = Op::op(acc, x.toFixnum());
        break;
      case TT_FLONUM:
        return calcf(state, i, static_cast<Flonum>(acc));
      default:
        state->runtimeError("Number expected, but `%@`", &x);
        break;
      }
    }
    return Value(acc);
  }

  static Value calcf(State* state, int i, Flonum acc) {
    int n = state->getArgNum();
    if (n == 1)
      return state->flonum(Op::single(acc));

    for (; i < n; ++i) {
      Value x = state->getArg(i);
      switch (x.getType()) {
      case TT_FIXNUM:
        acc = Op::op(acc, x.toFixnum());
        break;
      case TT_FLONUM:
        acc = Op::op(acc, x.toFlonum(state));
        break;
      default:
        state->runtimeError("Number expected, but `%@`", &x);
        break;
      }
    }
    return state->flonum(acc);
  }
};

struct Add {
  static Fixnum base()  { return 0; }
  template <class X> static X single(X x)  { return x; }
  template <class X, class Y> static X op(X x, Y y)  { return x + y; }
};
struct Sub {
  static Fixnum base()  { return 0; }
  template <class X> static X single(X x)  { return -x; }
  template <class X, class Y> static X op(X x, Y y)  { return x - y; }
};
struct Mul {
  static Fixnum base()  { return 1; }
  template <class X> static X single(X x)  { return x; }
  template <class X, class Y> static X op(X x, Y y)  { return x * y; }
};
struct Div {
  static Fixnum base()  { return 1; }
  template <class X> static X single(X x)  { return 1 / x; }
  template <class X, class Y> static X op(X x, Y y)  { return x / y; }
};

template <class Op>
struct CompareOp {
  static Value calc(State* state) {
    int n = state->getArgNum();
    assert(n >= 1);
    Value x = state->getArg(0);
    Fixnum acc;
    switch (x.getType()) {
    case TT_FIXNUM:
      acc = x.toFixnum();
      break;
    case TT_FLONUM:
      return calcf(state, 1, x.toFlonum(state));
      break;
    default:
      state->runtimeError("Number expected, but `%@`", &x);
      break;
    }

    for (int i = 1; i < n; ++i) {
      Value x = state->getArg(i);
      switch (x.getType()) {
      case TT_FIXNUM:
        {
          Fixnum xx = x.toFixnum();
          if (!Op::satisfy(acc, xx))
            return state->boolean(false);
          acc = xx;
        }
        break;
      case TT_FLONUM:
        return calcf(state, i, static_cast<Flonum>(acc));
      default:
        state->runtimeError("Number expected, but `%@`", &x);
        break;
      }
    }
    return state->boolean(true);
  }

  static Value calcf(State* state, int i, Flonum acc) {
    int n = state->getArgNum();
    for (; i < n; ++i) {
      Value x = state->getArg(i);
      switch (x.getType()) {
      case TT_FIXNUM:
        {
          Fixnum xx = x.toFixnum();
          if (!Op::satisfy(acc, xx))
            return state->boolean(false);
          acc = xx;
        }
        break;
      case TT_FLONUM:
        {
          Flonum xx = x.toFlonum(state);
          if (!Op::satisfy(acc, xx))
            return state->boolean(false);
          acc = xx;
        }
        break;
      default:
        state->runtimeError("Number expected, but `%@`", &x);
        break;
      }
    }
    return state->boolean(true);
  }
};

struct LessThan {
  template <class X, class Y> static bool satisfy(X x, Y y)  { return x < y; }
};
struct GreaterThan {
  template <class X, class Y> static bool satisfy(X x, Y y)  { return x > y; }
};
struct LessEqual {
  template <class X, class Y> static bool satisfy(X x, Y y)  { return x <= y; }
};
struct GreaterEqual {
  template <class X, class Y> static bool satisfy(X x, Y y)  { return x >= y; }
};

static Value s_add(State* state) {
  return BinOp<Add>::calc(state);
}

static Value s_sub(State* state) {
  return BinOp<Sub>::calc(state);
}

static Value s_mul(State* state) {
  return BinOp<Mul>::calc(state);
}

static Value s_div(State* state) {
  return BinOp<Div>::calc(state);
}

} // namespace

static Value s_lessThan(State* state) {
  return CompareOp<LessThan>::calc(state);
}

static Value s_greaterThan(State* state) {
  return CompareOp<GreaterThan>::calc(state);
}

static Value s_lessEqual(State* state) {
  return CompareOp<LessEqual>::calc(state);
}

static Value s_greaterEqual(State* state) {
  return CompareOp<GreaterEqual>::calc(state);
}

static Value s_flonum(State* state) {
  Value v = state->getArg(0);
  switch (v.getType()) {
  case TT_FLONUM:
    return v;
  case TT_FIXNUM:
    {
      Fixnum i = v.toFixnum();
      return state->flonum(static_cast<Flonum>(i));
    }
  case TT_STRING:
    {
      String* string = static_cast<String*>(v.toObject());
      double d = atof(string->c_str());
      return state->flonum(static_cast<Flonum>(d));
    }
  default:
    break;
  }
  state->runtimeError("Cannot convert `%@` to flonum", &v);
  return v;
}

static Value s_mod(State* state) {
  Value a = state->getArg(0);
  Value b = state->getArg(1);
  switch (a.getType()) {
  case TT_FIXNUM:
    switch (b.getType()) {
    case TT_FIXNUM:
      return Value(a.toFixnum() % b.toFixnum());
    case TT_FLONUM:
      return state->flonum(fmod(a.toFixnum(), b.toFlonum(state)));
    default:
      state->runtimeError("Number expected, but `%@`", &b);
      break;
    }
    break;
  case TT_FLONUM:
    switch (b.getType()) {
    case TT_FIXNUM:
      return state->flonum(fmod(a.toFlonum(state), b.toFixnum()));
    case TT_FLONUM:
      return state->flonum(fmod(a.toFlonum(state), b.toFlonum(state)));
    default:
      state->runtimeError("Number expected, but `%@`", &b);
      break;
    }
    break;
  default:
    state->runtimeError("Number expected, but `%@`", &a);
    break;
  }
  return Value::NIL;
}
#endif

void installFlonumFunctions(State* state) {
#ifndef DISABLE_FLONUM
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  struct {
    const char* name;
    NativeFuncType func;
    int minArgNum, maxArgNum;
  } static const FuncTable[] = {
    { "flonum", s_flonum, 1 },
    { "+", s_add, 0, -1 },
    { "-", s_sub, 0, -1 },
    { "*", s_mul, 0, -1 },
    { "/", s_div, 0, -1 },
    { "mod", s_mod, 2 },
    { "<", s_lessThan, 2, -1 },
    { ">", s_greaterThan, 2, -1 },
    { "<=", s_lessEqual, 2, -1 },
    { ">=", s_greaterEqual, 2, -1 },
  };

  for (auto it : FuncTable) {
    int maxArgNum = it.maxArgNum == 0 ? it.minArgNum : it.maxArgNum;
    state->defineNative(it.name, it.func, it.minArgNum, maxArgNum);
  }

  {
    bind::Binder b(state);
    b.bind("sin", (Flonum (*)(Flonum)) sin);
    b.bind("cos", (Flonum (*)(Flonum)) cos);
    b.bind("tan", (Flonum (*)(Flonum)) tan);
    b.bind("sqrt", (Flonum (*)(Flonum)) sqrt);
    b.bind("log", (Flonum (*)(Flonum)) log);
    b.bind("floor", (Flonum (*)(Flonum)) floor);
    b.bind("ceil", (Flonum (*)(Flonum)) ceil);
    b.bind("atan2", (Flonum (*)(Flonum, Flonum)) atan2);
    b.bind("expt", (Flonum (*)(Flonum, Flonum)) pow);
  }
#else
  (void)state;  // To avoid warning.
#endif
}

}  // namespace yalp
