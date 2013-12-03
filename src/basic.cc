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
      return calcf(state, 1, x.toFloat());
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
        acc = Op::op(acc, x.toFloat());
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

static Svalue s_read(State* state) {
  Reader reader(state, std::cin);
  Svalue exp;
  ReadError err = reader.read(&exp);
  if (err != READ_SUCCESS)
    state->runtimeError("Read error");
  return exp;
}

static Svalue s_make_hash_table(State* state) {
  return state->createHashTable();
}

static Svalue s_hash_table_get(State* state) {
  Svalue h = state->getArg(0);
  Svalue key = state->getArg(1);
  if (h.getType() != TT_HASH_TABLE) {
    state->runtimeError("Hash table expected");
  }
  const Svalue* result = static_cast<SHashTable*>(h.toObject())->get(key);
  if (result == NULL)
    return state->nil();
  return *result;
}

static Svalue s_hash_table_put(State* state) {
  Svalue h = state->getArg(0);
  Svalue key = state->getArg(1);
  Svalue value = state->getArg(2);
  if (h.getType() != TT_HASH_TABLE) {
    state->runtimeError("Hash table expected");
  }
  static_cast<SHashTable*>(h.toObject())->put(key, value);
  return value;
}

static Svalue s_hash_table_exists(State* state) {
  Svalue h = state->getArg(0);
  Svalue key = state->getArg(1);
  if (h.getType() != TT_HASH_TABLE) {
    state->runtimeError("Hash table expected");
  }
  Svalue result;
  return state->boolValue(static_cast<SHashTable*>(h.toObject())->get(key) != NULL);
}

static Svalue s_hash_table_delete(State* state) {
  Svalue h = state->getArg(0);
  Svalue key = state->getArg(1);
  if (h.getType() != TT_HASH_TABLE) {
    state->runtimeError("Hash table expected");
  }
  return state->boolValue(static_cast<SHashTable*>(h.toObject())->remove(key));
}

static Svalue s_hash_table_keys(State* state) {
  Svalue h = state->getArg(0);
  if (h.getType() != TT_HASH_TABLE) {
    state->runtimeError("Hash table expected");
  }

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
  state->assignGlobal(state->nil(), state->nil());
  state->assignGlobal(state->t(), state->t());

  state->assignNative("cons", s_cons, 2);
  state->assignNative("car", s_car, 1);
  state->assignNative("cdr", s_cdr, 1);
  state->assignNative("set-car!", s_set_car, 2);
  state->assignNative("set-cdr!", s_set_cdr, 2);
  state->assignNative("list", s_list, 0, -1);
  state->assignNative("list*", s_listStar, 0, -1);
  state->assignNative("consp", s_consp, 1);
  state->assignNative("symbolp", s_symbolp, 1);
  state->assignNative("append", s_append, 0, -1);
  state->assignNative("+", s_add, 0, -1);
  state->assignNative("-", s_sub, 0, -1);
  state->assignNative("*", s_mul, 0, -1);
  state->assignNative("/", s_div, 0, -1);

  state->assignNative("is", s_is, 2);
  state->assignNative("iso", s_iso, 2);
  state->assignNative("<", s_lessThan, 2, -1);
  state->assignNative(">", s_greaterThan, 2, -1);
  state->assignNative("<=", s_lessEqual, 2, -1);
  state->assignNative(">=", s_greaterEqual, 2, -1);

  state->assignNative("display", s_display, 1);
  state->assignNative("write", s_write, 1);

  state->assignNative("uniq", s_uniq, 0);
  state->assignNative("apply", s_apply, 1, -1);
  state->assignNative("read", s_read, 0);

  state->assignNative("make-hash-table", s_make_hash_table, 0);
  state->assignNative("hash-table-get", s_hash_table_get, 2);
  state->assignNative("hash-table-put!", s_hash_table_put, 3);
  state->assignNative("hash-table-exists?", s_hash_table_exists, 2);
  state->assignNative("hash-table-delete!", s_hash_table_delete, 2);
  state->assignNative("hash-table-keys", s_hash_table_keys, 1);

  state->assignNative("collect-garbage", s_collect_garbage, 0);
}

}  // namespace yalp
