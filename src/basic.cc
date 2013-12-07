//=============================================================================
/// basic - basic functions
//=============================================================================

#include "basic.hh"
#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "yalp/util.hh"
#include "hash_table.hh"
#include "vm.hh"
#include <assert.h>
#include <iostream>
#include <math.h>

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

static Svalue s_set_car(State* state) {
  Svalue s = state->getArg(0);
  Svalue value = state->getArg(1);
  static_cast<Cell*>(s.toObject())->setCar(value);
  return value;
}

static Svalue s_set_cdr(State* state) {
  Svalue s = state->getArg(0);
  Svalue value = state->getArg(1);
  static_cast<Cell*>(s.toObject())->setCdr(value);
  return value;
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
  if (n <= 0)
    res = state->nil();
  else {
    res = state->getArg(n - 1);
    for (int i = n - 1; --i >= 0; ) {
      Svalue a = state->getArg(i);
      res = state->cons(a, res);
    }
  }
  return res;
}

static Svalue s_pairp(State* state) {
  Svalue a = state->getArg(0);
  return state->boolValue(a.getType() == TT_CELL);
}

static Svalue s_symbolp(State* state) {
  Svalue a = state->getArg(0);
  return state->boolValue(a.getType() == TT_SYMBOL);
}

static Svalue s_append(State* state) {
  int n = state->getArgNum();
  Svalue nil = state->nil();
  Svalue last = nil;
  int lastIndex;
  for (lastIndex = n; --lastIndex >= 0; ) {
    last = state->getArg(lastIndex);
    if (!last.eq(nil))
      break;
  }
  if (lastIndex < 0)
    return nil;

  Svalue copied = nil;
  for (int i = 0; i < lastIndex; ++i) {
    Svalue ls = state->getArg(i);
    for (; ls.getType() == TT_CELL; ls = state->cdr(ls))
      copied = state->cons(state->car(ls), copied);
  }
  if (copied.eq(nil))
    return last;

  Svalue fin = nreverse(state, copied);
  static_cast<Cell*>(copied.toObject())->setCdr(last);
  return fin;
}

template <class Op>
struct BinOp {
  static Svalue calc(State* state) {
    int n = state->getArgNum();
    if (n <= 0)
      return state->fixnumValue(Op::base());
    Svalue x = state->getArg(0);
    Sfixnum acc;
    switch (x.getType()) {
    case TT_FIXNUM:
      acc = x.toFixnum();
      break;
    case TT_FLOAT:
      return calcf(state, 1, x.toFloat(state));
      break;
    default:
      state->runtimeError("Number expected");
      break;
    }
    if (n == 1)
      return state->fixnumValue(Op::single(acc));

    for (int i = 1; i < n; ++i) {
      Svalue x = state->getArg(i);
      switch (x.getType()) {
      case TT_FIXNUM:
        acc = Op::op(acc, x.toFixnum());
        break;
      case TT_FLOAT:
        return calcf(state, i, static_cast<Sfloat>(acc));
      default:
        state->runtimeError("Number expected");
        break;
      }
    }
    return state->fixnumValue(acc);
  }

  static Svalue calcf(State* state, int i, Sfloat acc) {
    int n = state->getArgNum();
    if (n == 1)
      return state->floatValue(Op::single(acc));

    for (; i < n; ++i) {
      Svalue x = state->getArg(i);
      switch (x.getType()) {
      case TT_FIXNUM:
        acc = Op::op(acc, x.toFixnum());
        break;
      case TT_FLOAT:
        acc = Op::op(acc, x.toFloat(state));
        break;
      default:
        state->runtimeError("Number expected");
        break;
      }
    }
    return state->floatValue(acc);
  }
};

static Svalue s_add(State* state) {
  struct Add {
    static Sfixnum base()  { return 0; }
    template <class X> static X single(X x)  { return x; }
    template <class X, class Y> static X op(X x, Y y)  { return x + y; }
  };
  return BinOp<Add>::calc(state);
}

static Svalue s_sub(State* state) {
  struct Sub {
    static Sfixnum base()  { return 0; }
    template <class X> static X single(X x)  { return -x; }
    template <class X, class Y> static X op(X x, Y y)  { return x - y; }
  };
  return BinOp<Sub>::calc(state);
}

static Svalue s_mul(State* state) {
  struct Mul {
    static Sfixnum base()  { return 1; }
    template <class X> static X single(X x)  { return x; }
    template <class X, class Y> static X op(X x, Y y)  { return x * y; }
  };
  return BinOp<Mul>::calc(state);
}

static Svalue s_div(State* state) {
  struct Div {
    static Sfixnum base()  { return 1; }
    template <class X> static X single(X x)  { return 1 / x; }
    template <class X, class Y> static X op(X x, Y y)  { return x / y; }
  };
  return BinOp<Div>::calc(state);
}

static Svalue s_is(State* state) {
  Svalue a = state->getArg(0);
  Svalue b = state->getArg(1);
  return state->boolValue(a.eq(b));
}

static Svalue s_iso(State* state) {
  Svalue a = state->getArg(0);
  Svalue b = state->getArg(1);
  return state->boolValue(a.equal(b));
}

template <class Op>
struct CompareOp {
  static Svalue calc(State* state) {
    int n = state->getArgNum();
    assert(n >= 1);
    Svalue x = state->getArg(0);
    Sfixnum acc;
    switch (x.getType()) {
    case TT_FIXNUM:
      acc = x.toFixnum();
      break;
    case TT_FLOAT:
      return calcf(state, 1, x.toFloat(state));
      break;
    default:
      state->runtimeError("Number expected");
      break;
    }

    for (int i = 1; i < n; ++i) {
      Svalue x = state->getArg(i);
      switch (x.getType()) {
      case TT_FIXNUM:
        {
          Sfixnum xx = x.toFixnum();
          if (!Op::satisfy(acc, xx))
            return state->boolValue(false);
          acc = xx;
        }
        break;
      case TT_FLOAT:
        return calcf(state, i, static_cast<Sfloat>(acc));
      default:
        state->runtimeError("Number expected");
        break;
      }
    }
    return state->boolValue(true);
  }

  static Svalue calcf(State* state, int i, Sfloat acc) {
    int n = state->getArgNum();
    for (; i < n; ++i) {
      Svalue x = state->getArg(i);
      switch (x.getType()) {
      case TT_FIXNUM:
        {
          Sfixnum xx = x.toFixnum();
          if (!Op::satisfy(acc, xx))
            return state->boolValue(false);
          acc = xx;
        }
        break;
      case TT_FLOAT:
        {
          Sfloat xx = x.toFloat(state);
          if (!Op::satisfy(acc, xx))
            return state->boolValue(false);
          acc = xx;
        }
        break;
      default:
        state->runtimeError("Number expected");
        break;
      }
    }
    return state->boolValue(true);
  }
};

static Svalue s_lessThan(State* state) {
  struct LessThan {
    template <class X, class Y> static bool satisfy(X x, Y y)  { return x < y; }
  };
  return CompareOp<LessThan>::calc(state);
}

static Svalue s_greaterThan(State* state) {
  struct GreaterThan {
    template <class X, class Y> static bool satisfy(X x, Y y)  { return x > y; }
  };
  return CompareOp<GreaterThan>::calc(state);
}

static Svalue s_lessEqual(State* state) {
  struct LessEqual {
    template <class X, class Y> static bool satisfy(X x, Y y)  { return x <= y; }
  };
  return CompareOp<LessEqual>::calc(state);
}

static Svalue s_greaterEqual(State* state) {
  struct GreaterEqual {
    template <class X, class Y> static bool satisfy(X x, Y y)  { return x >= y; }
  };
  return CompareOp<GreaterEqual>::calc(state);
}

template <typename Func>
Svalue FloatFunc1(State* state, Func f) {
  return state->floatValue(f(state->getArg(0).toFloat(state)));
}

template <typename Func>
Svalue FloatFunc2(State* state, Func f) {
  Sfloat x = state->getArg(0).toFloat(state);
  Sfloat y = state->getArg(1).toFloat(state);
  return state->floatValue(f(x, y));
}

static Svalue s_sin(State* state) { return FloatFunc1(state, sin); }
static Svalue s_cos(State* state) { return FloatFunc1(state, cos); }
static Svalue s_tan(State* state) { return FloatFunc1(state, tan); }
static Svalue s_sqrt(State* state) { return FloatFunc1(state, sqrt); }
static Svalue s_log(State* state) { return FloatFunc1(state, log); }
static Svalue s_floor(State* state) { return FloatFunc1(state, floor); }
static Svalue s_ceil(State* state) { return FloatFunc1(state, ceil); }
static Svalue s_atan2(State* state) { return FloatFunc2(state, atan2); }
static Svalue s_expt(State* state) { return FloatFunc2(state, pow); }

static Svalue s_write(State* state) {
  Svalue x = state->getArg(0);
  x.output(state, std::cout, true);
  return x;
}

static Svalue s_display(State* state) {
  Svalue x = state->getArg(0);
  x.output(state, std::cout, false);
  return x;
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
    if (last.eq(state->nil()))
      argNum -= 1;
    else if (last.getType() != TT_CELL)
      state->runtimeError("pair expected");
    else
      argNum += length(state, last) - 1;
  }

  Svalue* args = NULL;
  if (argNum > 0)
    args = static_cast<Svalue*>(alloca(sizeof(Svalue*) * argNum));
  for (int i = 0; i < argNum; ++i) {
    if (i < n - 2)
      args[i] = state->getArg(i + 1);
    else {
      args[i] = state->car(last);
      last = state->cdr(last);
    }
  }

  Svalue f = state->getArg(0);
  return state->tailcall(f, argNum, args);
}

static Svalue s_read(State* state) {
  Reader reader(state, std::cin);
  Svalue exp;
  ReadError err = reader.read(&exp);
  if (err != READ_SUCCESS)
    state->runtimeError("Read error");
  return exp;
}

static Svalue s_run_binary(State* state) {
  Svalue bin = state->getArg(0);
  Svalue result;
  if (!state->runBinary(bin, &result)) {
    std::cerr << "run_binary: failed" << std::endl;
  }
  return result;
}

static Svalue s_make_hash_table(State* state) {
  return state->createHashTable();
}

static Svalue s_hash_table_get(State* state) {
  Svalue h = state->getArg(0);
  Svalue key = state->getArg(1);
  if (h.getType() != TT_HASH_TABLE)
    state->runtimeError("Hash table expected");
  const Svalue* result = static_cast<SHashTable*>(h.toObject())->get(key);
  if (result == NULL)
    return state->nil();
  return *result;
}

static Svalue s_hash_table_put(State* state) {
  Svalue h = state->getArg(0);
  Svalue key = state->getArg(1);
  Svalue value = state->getArg(2);
  if (h.getType() != TT_HASH_TABLE)
    state->runtimeError("Hash table expected");
  static_cast<SHashTable*>(h.toObject())->put(key, value);
  return value;
}

static Svalue s_hash_table_exists(State* state) {
  Svalue h = state->getArg(0);
  Svalue key = state->getArg(1);
  if (h.getType() != TT_HASH_TABLE)
    state->runtimeError("Hash table expected");
  Svalue result;
  return state->boolValue(static_cast<SHashTable*>(h.toObject())->get(key) != NULL);
}

static Svalue s_hash_table_delete(State* state) {
  Svalue h = state->getArg(0);
  Svalue key = state->getArg(1);
  if (h.getType() != TT_HASH_TABLE)
    state->runtimeError("Hash table expected");
  return state->boolValue(static_cast<SHashTable*>(h.toObject())->remove(key));
}

static Svalue s_hash_table_keys(State* state) {
  Svalue h = state->getArg(0);
  if (h.getType() != TT_HASH_TABLE)
    state->runtimeError("Hash table expected");

  const SHashTable::TableType* ht = static_cast<SHashTable*>(h.toObject())->getHashTable();
  Svalue result = state->nil();
  for (auto kv : *ht)
    result = state->cons(kv.key, result);
  return result;
}

static Svalue s_collect_garbage(State* state) {
  state->collectGarbage();
  return state->nil();
}

void installBasicFunctions(State* state) {
  state->defineGlobal(state->nil(), state->nil());
  state->defineGlobal(state->t(), state->t());

  state->defineNative("cons", s_cons, 2);
  state->defineNative("car", s_car, 1);
  state->defineNative("cdr", s_cdr, 1);
  state->defineNative("set-car!", s_set_car, 2);
  state->defineNative("set-cdr!", s_set_cdr, 2);
  state->defineNative("list", s_list, 0, -1);
  state->defineNative("list*", s_listStar, 0, -1);
  state->defineNative("pair?", s_pairp, 1);
  state->defineNative("symbol?", s_symbolp, 1);
  state->defineNative("append", s_append, 0, -1);
  state->defineNative("+", s_add, 0, -1);
  state->defineNative("-", s_sub, 0, -1);
  state->defineNative("*", s_mul, 0, -1);
  state->defineNative("/", s_div, 0, -1);

  state->defineNative("is", s_is, 2);
  state->defineNative("iso", s_iso, 2);
  state->defineNative("<", s_lessThan, 2, -1);
  state->defineNative(">", s_greaterThan, 2, -1);
  state->defineNative("<=", s_lessEqual, 2, -1);
  state->defineNative(">=", s_greaterEqual, 2, -1);

  state->defineNative("sin", s_sin, 1);
  state->defineNative("cos", s_cos, 1);
  state->defineNative("tan", s_tan, 1);
  state->defineNative("sqrt", s_sqrt, 1);
  state->defineNative("log", s_log, 1);
  state->defineNative("floor", s_floor, 1);
  state->defineNative("ceil", s_ceil, 1);
  state->defineNative("atan2", s_atan2, 2);
  state->defineNative("expt", s_expt, 2);

  state->defineNative("display", s_display, 1);
  state->defineNative("write", s_write, 1);

  state->defineNative("uniq", s_uniq, 0);
  state->defineNative("apply", s_apply, 1, -1);
  state->defineNative("read", s_read, 0);
  state->defineNative("run-binary", s_run_binary, 1);

  state->defineNative("make-hash-table", s_make_hash_table, 0);
  state->defineNative("hash-table-get", s_hash_table_get, 2);
  state->defineNative("hash-table-put!", s_hash_table_put, 3);
  state->defineNative("hash-table-exists?", s_hash_table_exists, 2);
  state->defineNative("hash-table-delete!", s_hash_table_delete, 2);
  state->defineNative("hash-table-keys", s_hash_table_keys, 1);

  state->defineNative("collect-garbage", s_collect_garbage, 0);
}

}  // namespace yalp
