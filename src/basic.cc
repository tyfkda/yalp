//=============================================================================
/// basic - basic functions
//=============================================================================

#include "basic.hh"
#include "yalp.hh"
#include "yalp/object.hh"
#include "vm.hh"
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
  return state->car(cell);
}

static Svalue s_cdr(State* state) {
  Svalue cell = state->getArg(0);
  return state->cdr(cell);
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

static Svalue s_symbolp(State* state) {
  Svalue a = state->getArg(0);
  return state->boolValue(a.getType() == TT_SYMBOL);
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
        return state->cons(state->car(a), loop(state, state->cdr(a), d));
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

static Svalue s_apply(State* state) {
  int n = state->getArgNum();
  // Counts argument number for the given function.
  int argNum = n - 1;
  Svalue last = state->nil();
  if (n > 1) {
    // Last argument should be a list and its elements are function arguments.
    last = state->getArg(n - 1);
    if (last.eq(state->nil())) {
      argNum -= 1;
    } else if (last.getType() != TT_CELL) {
      state->runtimeError("pair expected");
    } else {
      argNum += length(state, last) - 1;
    }
  }

  Svalue* args = NULL;
  if (argNum > 0)
    args = static_cast<Svalue*>(alloca(sizeof(Svalue*) * argNum));
  for (int i = 0; i < argNum; ++i) {
    if (i < n - 2) {
      args[i] = state->getArg(i + 1);
    } else {
      args[i] = state->car(last);
      last = state->cdr(last);
    }
  }

  Svalue f = state->getArg(0);
  Svalue a = state->funcall(f, argNum, args);
  return a;
}

void installBasicFunctions(State* state) {
  state->assignGlobal(state->nil(), state->nil());
  state->assignGlobal(state->t(), state->t());

  state->assignNative("cons", s_cons, 2);
  state->assignNative("car", s_car, 1);
  state->assignNative("cdr", s_cdr, 1);
  state->assignNative("list", s_list, 0, -1);
  state->assignNative("list*", s_listStar, 0, -1);
  state->assignNative("consp", s_consp, 1);
  state->assignNative("symbolp", s_symbolp, 1);
  state->assignNative("append", s_append, 0, -1);
  state->assignNative("+", s_add, 0, -1);
  state->assignNative("-", s_sub, 0, -1);
  state->assignNative("*", s_mul, 0, -1);
  state->assignNative("/", s_div, 0, -1);

  state->assignNative("eq", s_eq, 2);
  state->assignNative("equal", s_equal, 2);
  state->assignNative("<", s_lessThan, 2, -1);
  state->assignNative(">", s_greaterThan, 2, -1);
  state->assignNative("<=", s_lessEqual, 2, -1);
  state->assignNative(">=", s_greaterEqual, 2, -1);

  state->assignNative("print", s_print, 1);
  state->assignNative("display", s_write, 1);
  state->assignNative("write", s_write, 1);
  state->assignNative("newline", s_newline, 0);

  state->assignNative("uniq", s_uniq, 0);
  state->assignNative("apply", s_apply, 1, -1);
}

}  // namespace yalp
