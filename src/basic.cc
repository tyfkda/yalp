//=============================================================================
/// basic - basic functions
//=============================================================================

#include "basic.hh"
#include "yalp.hh"
#include "yalp/binder.hh"
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"
#include "hash_table.hh"
#include "vm.hh"

#include <alloca.h>
#include <assert.h>
#include <tgmath.h>

using yalpbind::YalpBind;

namespace yalp {

//=============================================================================
// Native functions

// Expand macro if the given expression is macro expression,
// otherwise return itself.
/*
(def (macroexpand-1 exp)
  (if (and (pair? exp)
           (macro? (car exp)))
      (with (name (car exp)
             args (cdr exp))
        (let closure (hash-table-get *macro-table* name)
          (apply closure args)))
    exp))
*/
static Value s_macroexpand_1(State* state) {
  Value exp = state->getArg(0);
  if (exp.getType() != TT_CELL)
    return exp;
  Value name = state->car(exp);
  Value closure = state->getMacro(name);
  if (closure.isFalse())
    return exp;

  Value args = state->cdr(exp);
  int argNum = length(args);
  Value* argsArray = static_cast<Value*>(alloca(sizeof(Value) * argNum));
  for (int i = 0; i < argNum; ++i, args = state->cdr(args))
    argsArray[i] = state->car(args);
  return state->tailcall(closure, argNum, argsArray);
}

static Value s_type(State* state) {
  Value v = state->getArg(0);
  return state->getTypeSymbol(v.getType());
}

static Value s_cons(State* state) {
  Value a = state->getArg(0);
  Value d = state->getArg(1);
  return state->cons(a, d);
}

static Value s_car(State* state) {
  Value cell = state->getArg(0);
  return state->car(cell);
}

static Value s_cdr(State* state) {
  Value cell = state->getArg(0);
  return state->cdr(cell);
}

static Value s_set_car(State* state) {
  Value s = state->getArg(0);
  Value value = state->getArg(1);
  static_cast<Cell*>(s.toObject())->setCar(value);
  return value;
}

static Value s_set_cdr(State* state) {
  Value s = state->getArg(0);
  Value value = state->getArg(1);
  static_cast<Cell*>(s.toObject())->setCdr(value);
  return value;
}

static Value s_list(State* state) {
  int n = state->getArgNum();
  Value res = Value::NIL;
  for (int i = n; --i >= 0; ) {
    Value a = state->getArg(i);
    res = state->cons(a, res);
  }
  return res;
}

static Value s_listStar(State* state) {
  int n = state->getArgNum();
  Value res;
  if (n <= 0)
    res = Value::NIL;
  else {
    res = state->getArg(n - 1);
    for (int i = n - 1; --i >= 0; ) {
      Value a = state->getArg(i);
      res = state->cons(a, res);
    }
  }
  return res;
}

static Value s_append(State* state) {
  int n = state->getArgNum();
  Value nil = Value::NIL;
  Value last = nil;
  int lastIndex;
  for (lastIndex = n; --lastIndex >= 0; ) {
    last = state->getArg(lastIndex);
    if (!last.eq(nil))
      break;
  }
  if (lastIndex < 0)
    return nil;

  Value copied = nil;
  for (int i = 0; i < lastIndex; ++i) {
    Value ls = state->getArg(i);
    for (; ls.getType() == TT_CELL; ls = state->cdr(ls))
      copied = state->cons(state->car(ls), copied);
  }
  if (copied.eq(nil))
    return last;

  Value fin = nreverse(copied);
  static_cast<Cell*>(copied.toObject())->setCdr(last);
  return fin;
}

static Value s_nreverse(State* state) {
  return nreverse(state->getArg(0));
}

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

static Value s_is(State* state) {
  Value a = state->getArg(0);
  Value b = state->getArg(1);
  return state->boolean(a.eq(b));
}

static Value s_iso(State* state) {
  Value a = state->getArg(0);
  Value b = state->getArg(1);
  return state->boolean(a.equal(b));
}

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

static Stream* chooseStream(State* state, int argIndex, const char* defaultStreamName) {
  Value ss = state->getArgNum() > argIndex ?
    state->getArg(argIndex) :
    state->referGlobal(defaultStreamName);
  state->checkType(ss, TT_STREAM);
  return static_cast<SStream*>(ss.toObject())->getStream();
}

static Value output(State* state, bool inspect) {
  Value x = state->getArg(0);
  Stream* stream = chooseStream(state, 1, "*stdout*");
  x.output(state, stream, inspect);
  return x;
}

static Value s_write(State* state) {
  return output(state, true);
}

static Value s_display(State* state) {
  return output(state, false);
}

static Value s_format(State* state) {
  Stream* stream = chooseStream(state, 0, "*stdout*");
  Value fmt = state->getArg(1);
  state->checkType(fmt, TT_STRING);

  int argNum = state->getArgNum() - 2;
  Value* values = static_cast<Value*>(alloca(sizeof(Value) * argNum));
  for (int i = 0; i < argNum; ++i)
    values[i] = state->getArg(i + 2);
  format(state, stream, static_cast<String*>(fmt.toObject())->c_str(), values);
  return Value::NIL;
}

static Value s_read(State* state) {
  Stream* stream = chooseStream(state, 0, "*stdin*");
  Value eof = state->getArgNum() > 1 ? state->getArg(1) : Value::NIL;
  Reader reader(state, stream);
  Value exp;
  ErrorCode err = reader.read(&exp);
  switch (err) {
  case SUCCESS:
    return exp;
  case END_OF_FILE:
    return eof;
  default:
    state->runtimeError("Read error %d", err);
    return Value::NIL;
  }
}

static Value s_read_from_string(State* state) {
  Value s = state->getArg(0);
  state->checkType(s, TT_STRING);

  StrStream stream(static_cast<String*>(s.toObject())->c_str());
  Reader reader(state, &stream);
  Value exp;
  ErrorCode err = reader.read(&exp);
  if (err != SUCCESS)
    state->runtimeError("Read error %d", err);
  return exp;
}

static Value s_read_delimited_list(State* state) {
  Value delimiter = state->getArg(0);
  state->checkType(delimiter, TT_FIXNUM);  // Actually, CHAR
  Stream* stream = chooseStream(state, 1, "*stdin*");
  Reader reader(state, stream);
  Value result;
  ErrorCode err = reader.readDelimitedList(delimiter.toCharacter(), &result);
  if (err != SUCCESS)
    state->runtimeError("Read error %d", err);
  return result;
}

static Value s_read_char(State* state) {
  Stream* stream = chooseStream(state, 0, "*stdin*");
  int c = stream->get();
  return c == EOF ? Value::NIL : state->character(c);
}

static char* reallocateString(State* state, char* heap, int heapSize,
                              const char* local, int len) {
  // Copy local to heap.
  int newSize = heapSize + len;
  char* newHeap = static_cast<char*>(state->realloc(heap, sizeof(*heap) * newSize));
  // TODO: Handle memory over.
  memcpy(&newHeap[heapSize], local, sizeof(*heap) * len);
  return newHeap;
}

static Value s_read_line(State* state) {
  const int SIZE = 8;
  char local[SIZE];
  int len = 0;
  char* heap = NULL;
  int heapSize = 0;

  Stream* stream = chooseStream(state, 0, "*stdin*");
  int c;
  for (;;) {
    c = stream->get();
    if (c == EOF || c == '\n')
      break;
    if (len >= SIZE) {
      // Copy local to heap.
      heap = reallocateString(state, heap, heapSize,
                              local, len);
      heapSize += len;
      len = 0;
    }
    local[len++] = c;
  }

  if (heapSize == 0 && len < SIZE) {
    if (len == 0 && c == EOF)
      return Value::NIL;
    local[len] = '\0';
    return state->string(local);
  }

  char* copiedString = reallocateString(state, heap, heapSize,
                                        local, len + 1);
  len = heapSize + len;
  copiedString[len] = '\0';
  return state->allocatedString(copiedString, len);
}

static Value s_uniq(State* state) {
  return state->gensym();
}

static Value s_apply(State* state) {
  int n = state->getArgNum();
  // Counts argument number for the given function.
  int argNum = n - 1;
  Value last = Value::NIL;
  if (n > 1) {
    // Last argument should be a list and its elements are function arguments.
    last = state->getArg(n - 1);
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
      args[i] = state->getArg(i + 1);
    else {
      args[i] = state->car(last);
      last = state->cdr(last);
    }
  }

  Value f = state->getArg(0);
  return state->tailcall(f, argNum, args);
}

static Value s_run_binary(State* state) {
  Value bin = state->getArg(0);
  Value result;
  if (!state->runBinary(bin, &result))
    return Value::NIL;
  return result;
}

static Value s_make_hash_table(State* state) {
  return state->createHashTable();
}

static Value s_hash_table_get(State* state) {
  Value h = state->getArg(0);
  Value key = state->getArg(1);
  state->checkType(h, TT_HASH_TABLE);
  const Value* result = static_cast<SHashTable*>(h.toObject())->get(key);
  if (result == NULL)
    return Value::NIL;
  return *result;
}

static Value s_hash_table_put(State* state) {
  Value h = state->getArg(0);
  Value key = state->getArg(1);
  Value value = state->getArg(2);
  state->checkType(h, TT_HASH_TABLE);
  static_cast<SHashTable*>(h.toObject())->put(key, value);
  return value;
}

static Value s_hash_table_exists(State* state) {
  Value h = state->getArg(0);
  Value key = state->getArg(1);
  state->checkType(h, TT_HASH_TABLE);
  Value result;
  return state->boolean(static_cast<SHashTable*>(h.toObject())->get(key) != NULL);
}

static Value s_hash_table_delete(State* state) {
  Value h = state->getArg(0);
  Value key = state->getArg(1);
  state->checkType(h, TT_HASH_TABLE);
  return state->boolean(static_cast<SHashTable*>(h.toObject())->remove(key));
}

static Value s_hash_table_keys(State* state) {
  Value h = state->getArg(0);
  state->checkType(h, TT_HASH_TABLE);

  const SHashTable::TableType* ht = static_cast<SHashTable*>(h.toObject())->getHashTable();
  Value result = Value::NIL;
  for (auto kv : *ht)
    result = state->cons(kv.key, result);
  return result;
}

static Value s_set_macro_character(State* state) {
  Value chr = state->getArg(0);
  Value fn = state->getArg(1);
  state->setMacroCharacter(chr.toCharacter(), fn);
  return fn;
}

static Value s_get_macro_character(State* state) {
  Value chr = state->getArg(0);
  return state->getMacroCharacter(chr.toCharacter());
}

static Value s_collect_garbage(State* state) {
  state->collectGarbage();
  return state->getConstant(State::T);
}

static Value s_exit(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_FIXNUM);
  int code = v.toFixnum();
  exit(code);
  return Value::NIL;
}

static Value s_vmtrace(State* state) {
  Value v = state->getArg(0);
  bool b = v.isTrue();
  state->setVmTrace(b);
  return state->getConstant(State::T);
}

static Value s_open(State* state) {
  Value filespec = state->getArg(0);
  state->checkType(filespec, TT_STRING);
  const char* mode = "rb";
  if (state->getArgNum() > 1 && state->getArg(1).isTrue())
    mode = "wb";

  const char* path = static_cast<String*>(filespec.toObject())->c_str();
  FILE* fp = fopen(path, mode);
  if (fp == NULL)
    return Value::NIL;
  return state->createFileStream(fp);
}

static Value s_close(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_STREAM);

  SStream* ss = static_cast<SStream*>(v.toObject());
  Stream* stream = ss->getStream();
  return state->boolean(stream->close());
}

void installBasicFunctions(State* state) {
  state->defineGlobal(Value::NIL, Value::NIL);
  state->defineGlobal(state->getConstant(State::T), state->getConstant(State::T));

  state->defineNative("macroexpand-1", s_macroexpand_1, 1);
  state->defineNative("type", s_type, 1);
  state->defineNative("cons", s_cons, 2);
  state->defineNative("car", s_car, 1);
  state->defineNative("cdr", s_cdr, 1);
  state->defineNative("set-car!", s_set_car, 2);
  state->defineNative("set-cdr!", s_set_cdr, 2);
  state->defineNative("list", s_list, 0, -1);
  state->defineNative("list*", s_listStar, 0, -1);
  state->defineNative("append", s_append, 0, -1);
  state->defineNative("reverse!", s_nreverse, 1);
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

  {
    YalpBind b(state);
    b.bind("sin", sin);
    b.bind("cos", cos);
    b.bind("tan", tan);
    b.bind("sqrt", sqrt);
    b.bind("log", log);
    b.bind("floor", floor);
    b.bind("ceil", ceil);
    b.bind("atan2", atan2);
    b.bind("expt", pow);
  }

  state->defineNative("display", s_display, 1, 2);
  state->defineNative("write", s_write, 1, 2);
  state->defineNative("format", s_format, 2, -1);

  state->defineNative("read", s_read, 0, 2);
  state->defineNative("read-from-string", s_read_from_string, 1);
  state->defineNative("read-delimited-list", s_read_delimited_list, 2);
  state->defineNative("read-char", s_read_char, 0, 1);
  state->defineNative("read-line", s_read_line, 0, 1);

  state->defineNative("uniq", s_uniq, 0);
  state->defineNative("apply", s_apply, 1, -1);
  state->defineNative("run-binary", s_run_binary, 1);

  state->defineNative("make-hash-table", s_make_hash_table, 0);
  state->defineNative("hash-table-get", s_hash_table_get, 2);
  state->defineNative("hash-table-put!", s_hash_table_put, 3);
  state->defineNative("hash-table-exists?", s_hash_table_exists, 2);
  state->defineNative("hash-table-delete!", s_hash_table_delete, 2);
  state->defineNative("hash-table-keys", s_hash_table_keys, 1);

  state->defineNative("set-macro-character", s_set_macro_character, 2);
  state->defineNative("get-macro-character", s_get_macro_character, 1);

  state->defineNative("collect-garbage", s_collect_garbage, 0);
  state->defineNative("exit", s_exit, 1);
  state->defineNative("vmtrace", s_vmtrace, 1);

  state->defineNative("open", s_open, 1, 2);
  state->defineNative("close", s_close, 1);
}

}  // namespace yalp
