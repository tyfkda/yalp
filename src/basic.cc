//=============================================================================
/// basic - basic functions
//=============================================================================

#include "build_env.hh"
#include "basic.hh"
#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"
#include "hash_table.hh"
#include "vm.hh"

#include <assert.h>
#include <string.h>  // for memcpy
#include <stdlib.h>  // for exit

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
        (let closure (table-get *macro-table* name)
          (apply closure args)))
    exp))
*/
static Value s_macroexpand_1(State* state) {
  Value exp = state->getArg(0);
  if (exp.getType() != TT_CELL)
    return exp;
  Value name = car(exp);
  Value closure = state->getMacro(name);
  if (closure.isFalse())
    return exp;

  Value args = cdr(exp);
  int argNum = length(args);
  Value* argsArray = static_cast<Value*>(alloca(sizeof(Value) * argNum));
  for (int i = 0; i < argNum; ++i, args = cdr(args))
    argsArray[i] = car(args);
  return state->tailcall(closure, argNum, argsArray);
}

static Value s_globalVariableTable(State* state) {
  return Value(state->getGlobalVariableTable());
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
  return car(cell);
}

static Value s_cdr(State* state) {
  Value cell = state->getArg(0);
  return cdr(cell);
}

static Value s_setCar(State* state) {
  Value s = state->getArg(0);
  state->checkType(s, TT_CELL);
  Value value = state->getArg(1);
  static_cast<Cell*>(s.toObject())->setCar(value);
  return value;
}

static Value s_setCdr(State* state) {
  Value s = state->getArg(0);
  state->checkType(s, TT_CELL);
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
  Value last = Value::NIL;
  int lastIndex;
  for (lastIndex = n; --lastIndex >= 0; ) {
    last = state->getArg(lastIndex);
    if (!last.eq(Value::NIL))
      break;
  }
  if (lastIndex < 0)
    return Value::NIL;

  Value copied = Value::NIL;
  for (int i = 0; i < lastIndex; ++i) {
    Value ls = state->getArg(i);
    for (; ls.getType() == TT_CELL; ls = cdr(ls))
      copied = state->cons(car(ls), copied);
  }
  if (copied.eq(Value::NIL))
    return last;

  Value fin = nreverse(copied);
  static_cast<Cell*>(copied.toObject())->setCdr(last);
  return fin;
}

static Value s_appendBang(State* state) {
  int n = state->getArgNum();
  Value last = Value::NIL;
  int lastIndex;
  for (lastIndex = n; --lastIndex >= 0; ) {
    last = state->getArg(lastIndex);
    if (!last.eq(Value::NIL))
      break;
  }
  if (lastIndex < 0)
    return Value::NIL;

  for (int i = lastIndex; --i >= 0; ) {
    Value ls = state->getArg(i);
    if (ls.eq(Value::NIL))
      continue;
    state->checkType(ls, TT_CELL);
    for (Value p = ls;;) {
      Value d = cdr(p);
      if (d.getType() != TT_CELL) {
        static_cast<Cell*>(p.toObject())->setCdr(last);
        break;
      }
      p = d;
    }
    last = ls;
  }
  return last;
}

static Value s_reverseBang(State* state) {
  return nreverse(state->getArg(0));
}

static Value s_eq(State* state) {
  Value a = state->getArg(0);
  Value b = state->getArg(1);
  return state->boolean(a.eq(b));
}

static Value s_equal(State* state) {
  Value a = state->getArg(0);
  Value b = state->getArg(1);
  return state->boolean(a.equal(b));
}

namespace {

template <class Op>
struct BinOp {
  static Value calc(State* state) {
    int n = state->getArgNum();
    if (n <= 0)
      return Value(Op::base());
    Value x = state->getArg(0);
    state->checkType(x, TT_FIXNUM);
    Fixnum acc = x.toFixnum();
    if (n == 1)
      return Value(Op::single(acc));

    for (int i = 1; i < n; ++i) {
      Value x = state->getArg(i);
      state->checkType(x, TT_FIXNUM);
      acc = Op::op(acc, x.toFixnum());
    }
    return Value(acc);
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
    state->checkType(x, TT_FIXNUM);
    Fixnum acc = x.toFixnum();

    for (int i = 1; i < n; ++i) {
      Value x = state->getArg(i);
      state->checkType(x, TT_FIXNUM);
      Fixnum xx = x.toFixnum();
      if (!Op::satisfy(acc, xx))
        return state->boolean(false);
      acc = xx;
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

}  // namespace

Value s_add(State* state) {
  return BinOp<Add>::calc(state);
}

Value s_sub(State* state) {
  return BinOp<Sub>::calc(state);
}

Value s_mul(State* state) {
  return BinOp<Mul>::calc(state);
}

Value s_div(State* state) {
  return BinOp<Div>::calc(state);
}

static Value s_mod(State* state) {
  Value a = state->getArg(0);
  state->checkType(a, TT_FIXNUM);
  Value b = state->getArg(1);
  state->checkType(b, TT_FIXNUM);
  return Value(a.toFixnum() % b.toFixnum());
}

Value s_lessThan(State* state) {
  return CompareOp<LessThan>::calc(state);
}

Value s_greaterThan(State* state) {
  return CompareOp<GreaterThan>::calc(state);
}

Value s_lessEqual(State* state) {
  return CompareOp<LessEqual>::calc(state);
}

Value s_greaterEqual(State* state) {
  return CompareOp<GreaterEqual>::calc(state);
}

// Logical operator: Handles fixnum value only.
template <class Op>
struct LogiOp {
  static Value calc(State* state) {
    Fixnum value = Op::base();
    for (int n = state->getArgNum(), i = 0; i < n; ++i) {
      Value v = state->getArg(i);
      state->checkType(v, TT_FIXNUM);
      value = Op::calc(value, v.toFixnum());
    }
    return Value(value);
  }
};

struct LogicalAnd {
  static Fixnum base()  { return -1; }
  static Fixnum calc(Fixnum a, Fixnum b)  { return a & b; }
};
struct LogicalOr {
  static Fixnum base()  { return 0; }
  static Fixnum calc(Fixnum a, Fixnum b)  { return a | b; }
};
struct LogicalXor {
  static Fixnum base()  { return 0; }
  static Fixnum calc(Fixnum a, Fixnum b)  { return a ^ b; }
};

static Value s_logand(State* state) {
  return LogiOp<LogicalAnd>::calc(state);
}

static Value s_logior(State* state) {
  return LogiOp<LogicalOr>::calc(state);
}

static Value s_logxor(State* state) {
  return LogiOp<LogicalXor>::calc(state);
}

// Arithmetic shift.
static Value s_ash(State* state) {
  Value x = state->getArg(0);
  Value shift = state->getArg(1);
  state->checkType(x, TT_FIXNUM);
  state->checkType(shift, TT_FIXNUM);
  int s = shift.toFixnum();
  if (s >= 0)
    return Value(x.toFixnum() << s);
  else
    return Value(x.toFixnum() >> -s);
}

static SStream* chooseStream(State* state, int argIndex, State::Constant defaultStream) {
  Value ss = state->getArgNum() > argIndex ?
    state->getArg(argIndex) :
    state->referGlobal(state->getConstant(defaultStream));
  state->checkType(ss, TT_STREAM);
  return static_cast<SStream*>(ss.toObject());
}

static Value output(State* state, bool inspect) {
  Value x = state->getArg(0);
  Stream* stream = chooseStream(state, 1, State::STDOUT)->getStream();
  x.output(state, stream, inspect);
  return x;
}

static Value s_write(State* state) {
  return output(state, true);
}

static Value s_display(State* state) {
  return output(state, false);
}

static Value s_print(State* state) {
  Value x = state->getArg(0);
  Stream* stream = chooseStream(state, 1, State::STDOUT)->getStream();
  x.output(state, stream, false);
  stream->write("\n");
  return x;
}

static Value s_format(State* state) {
  Value ss = state->getArg(0);
  Stream* stream;
  StrOStream* ostream = NULL;
  if (ss.eq(Value::NIL)) {
    void* memory = alloca(sizeof(StrOStream));
    stream = ostream = new(memory) StrOStream(state->getAllocator());
  } else {
    state->checkType(ss, TT_STREAM);
    stream = static_cast<SStream*>(ss.toObject())->getStream();
  }
  Value fmt = state->getArg(1);
  state->checkType(fmt, TT_STRING);

  int argNum = state->getArgNum() - 2;
  Value* values = static_cast<Value*>(alloca(sizeof(Value) * argNum));
  for (int i = 0; i < argNum; ++i)
    values[i] = state->getArg(i + 2);
  format(state, stream, static_cast<String*>(fmt.toObject())->c_str(), values);
  if (ostream != NULL) {
    ostream->close();
    Value s = state->string(ostream->getString(), ostream->getLength());
    ostream->~StrOStream();
    return s;
  }
  return state->multiValues();
}

static Value doRead(State* state, Reader* reader, Value* eof) {
  Value exp;
  ErrorCode err = reader->read(&exp);
  switch (err) {
  case SUCCESS:
    return exp;
  case END_OF_FILE:
    if (eof != NULL)
      return *eof;
    break;
  default:
    break;
  }
  raiseReadError(state, err, reader);
  return Value::NIL;
}

static Value s_read(State* state) {
  Stream* stream = chooseStream(state, 0, State::STDIN)->getStream();
  Value eof = state->getArgNum() > 1 ? state->getArg(1) : Value::NIL;
  Reader reader(state, stream);
  return doRead(state, &reader, &eof);
}

static Value s_readFromString(State* state) {
  Value s = state->getArg(0);
  state->checkType(s, TT_STRING);

  StrStream stream(static_cast<String*>(s.toObject())->c_str());
  Reader reader(state, &stream);
  Value result = doRead(state, &reader, NULL);
  return state->multiValues(result, Value(stream.getIndex()));
}

static Value s_readDelimitedList(State* state) {
  Value delimiter = state->getArg(0);
  state->checkType(delimiter, TT_CHAR);
  Stream* stream = chooseStream(state, 1, State::STDIN)->getStream();
  Reader reader(state, stream);
  Value result;
  ErrorCode err = reader.readDelimitedList(delimiter.toCharacter(), &result);
  if (err != SUCCESS)
    state->runtimeError("Read error %d", err);
  return result;
}

static Value s_readChar(State* state) {
  Stream* stream = chooseStream(state, 0, State::STDIN)->getStream();
  int c = stream->get();
  return c == EOF ? Value::NIL : state->character(c);
}

static Value s_unreadChar(State* state) {
  Value ch = state->getArg(0);
  state->checkType(ch, TT_CHAR);
  SStream* sstream = chooseStream(state, 1, State::STDIN);
  sstream->getStream()->ungetc(ch.toCharacter());
  return Value(sstream);
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

static Value s_readLine(State* state) {
  const int SIZE = 8;
  char local[SIZE];
  int len = 0;
  char* heap = NULL;
  int heapSize = 0;

  Stream* stream = chooseStream(state, 0, State::STDIN)->getStream();
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

static Value s_gensym(State* state) {
  return state->gensym();
}

static Value s_apply(State* state) {
  return state->applyFunction();
}

static Value s_runBinary(State* state) {
  Value bin = state->getArg(0);
  Value result;
  if (!state->runBinary(bin, &result))
    return Value::NIL;
  return result;
}

static Value s_load(State* state) {
  Value fn = state->getArg(0);
  state->checkType(fn, TT_STRING);
  String* fileName = static_cast<String*>(fn.toObject());
  Value result;
  ErrorCode err = state->runFromFile(fileName->c_str(), &result);
  if (err != SUCCESS)
    state->runtimeError("Cannot load \"%s\"", fileName->c_str());
  return result;
}

static Value s_loadBinary(State* state) {
  Value fn = state->getArg(0);
  state->checkType(fn, TT_STRING);
  String* fileName = static_cast<String*>(fn.toObject());
  Value result;
  ErrorCode err = state->runBinaryFromFile(fileName->c_str(), &result);
  if (err != SUCCESS)
    state->runtimeError("Cannot load binary \"%s\"", fileName->c_str());
  return result;
}

static Value s_error(State* state) {
  FileStream out(stderr);
  Value fmt = state->getArg(0);
  state->checkType(fmt, TT_STRING);

  int argNum = state->getArgNum() - 1;
  Value* values = static_cast<Value*>(alloca(sizeof(Value) * argNum));
  for (int i = 0; i < argNum; ++i)
    values[i] = state->getArg(i + 1);
  format(state, &out, static_cast<String*>(fmt.toObject())->c_str(), values);
  state->runtimeError("");
  return state->multiValues();
}

static Value s_table(State* state) {
  bool equal = true;
  if (state->getArgNum() > 0) {
    Value type = state->getArg(0);
    if (type.eq(state->intern("eq?")))
      equal = false;
    else if (type.eq(state->intern("equal?")))
      equal = true;
    else
      state->runtimeError("Illegal compare type `%@`", &type);
  }
  return Value(state->createHashTable(equal));
}

static Value s_tableGet(State* state) {
  Value h = state->getArg(0);
  Value key = state->getArg(1);
  state->checkType(h, TT_HASH_TABLE);
  const Value* result = static_cast<SHashTable*>(h.toObject())->get(key);
  if (result == NULL) {
    Value v = state->getArgNum() > 2 ? state->getArg(2) : Value::NIL;
    return state->multiValues(v, Value::NIL);
  }
  return state->multiValues(*result, state->getConstant(State::T));
}

static Value s_tablePut(State* state) {
  Value h = state->getArg(0);
  Value key = state->getArg(1);
  Value value = state->getArg(2);
  state->checkType(h, TT_HASH_TABLE);
  static_cast<SHashTable*>(h.toObject())->put(key, value);
  return value;
}

static Value s_tableExists(State* state) {
  Value h = state->getArg(0);
  Value key = state->getArg(1);
  state->checkType(h, TT_HASH_TABLE);
  Value result;
  return state->boolean(static_cast<SHashTable*>(h.toObject())->get(key) != NULL);
}

static Value s_tableDelete(State* state) {
  Value h = state->getArg(0);
  Value key = state->getArg(1);
  state->checkType(h, TT_HASH_TABLE);
  return state->boolean(static_cast<SHashTable*>(h.toObject())->remove(key));
}

static Value s_tableKeys(State* state) {
  Value h = state->getArg(0);
  state->checkType(h, TT_HASH_TABLE);

  const SHashTable::TableType* ht = static_cast<SHashTable*>(h.toObject())->getHashTable();
  Value result = Value::NIL;
  int arena = state->saveArena();
  for (auto kv : *ht) {
    result = state->cons(kv.key, result);
    state->restoreArenaWith(arena, result);
  }
  return result;
}

static Value s_makeVector(State* state) {
  Value ssize = state->getArg(0);
  state->checkType(ssize, TT_FIXNUM);
  int size = ssize.toFixnum();
  Value fillValue = state->getArgNum() > 1 ? state->getArg(1) : Value::NIL;
  Allocator* allocator = state->getAllocator();
  Vector* vector = allocator->newObject<Vector>(allocator, size);
  for (int i = 0; i < size; ++i)
    vector->set(i, fillValue);
  return Value(vector);
}

static Value s_vector(State* state) {
  int size = state->getArgNum();
  Allocator* allocator = state->getAllocator();
  Vector* vector = allocator->newObject<Vector>(allocator, size);
  for (int i = 0; i < size; ++i)
    vector->set(i, state->getArg(i));
  return Value(vector);
}

static Value s_vectorLength(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_VECTOR);
  Vector* vector = static_cast<Vector*>(v.toObject());
  return Value(vector->size());
}

static Value s_vectorGet(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_VECTOR);
  Vector* vector = static_cast<Vector*>(v.toObject());
  Value i = state->getArg(1);
  state->checkType(i, TT_FIXNUM);
  int index = i.toFixnum();
  if (index < 0 || index >= vector->size())
    state->runtimeError("Out of vector size, %d/%d", vector->size(), index);
  return vector->get(index);
}

static Value s_vectorSet(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_VECTOR);
  Vector* vector = static_cast<Vector*>(v.toObject());
  Value i = state->getArg(1);
  state->checkType(i, TT_FIXNUM);
  int index = i.toFixnum();
  if (index < 0 || index >= vector->size())
    state->runtimeError("Out of vector size, %d/%d", vector->size(), index);
  Value value = state->getArg(2);
  vector->set(index, value);
  return value;
}

static Value s_setMacroCharacter(State* state) {
  Value chr = state->getArg(0);
  Value fn = state->getArg(1);
  state->setMacroCharacter(chr.toCharacter(), fn);
  return fn;
}

static Value s_getMacroCharacter(State* state) {
  Value chr = state->getArg(0);
  return state->getMacroCharacter(chr.toCharacter());
}

static Value s_collectGarbage(State* state) {
  state->collectGarbage();
  return state->getConstant(State::T);
}

static Value s_exit(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_FIXNUM);
  int code = v.toFixnum();
  exit(code);
  return state->multiValues();
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

  Stream* stream = static_cast<SStream*>(v.toObject())->getStream();
  return state->boolean(stream->close());
}

static Value s_int(State* state) {
  Value v = state->getArg(0);
  switch (v.getType()) {
  case TT_FIXNUM:
    return v;
#ifndef DISABLE_FLONUM
  case TT_FLONUM:
    {
      Flonum f = v.toFlonum(state);
      return Value(static_cast<Fixnum>(f));
    }
#endif
  case TT_CHAR:
    return Value(v.toCharacter());
  case TT_STRING:
    {
      String* string = static_cast<String*>(v.toObject());
      long l = atol(string->c_str());
      return Value(l);
    }
  default:
    break;
  }
  state->runtimeError("Cannot convert `%@` to int", &v);
  return v;
}

static Value s_char(State* state) {
  Value v = state->getArg(0);
  switch (v.getType()) {
  case TT_FIXNUM:
    return state->character(v.toFixnum());
  case TT_CHAR:
    return v;
  default:
    state->runtimeError("Cannot convert `%@` to char", &v);
    return v;
  }
}

static Value s_string(State* state) {
  int n = state->getArgNum();
  if (n == 1) {
    Value v = state->getArg(0);
    if (v.getType() == TT_STRING)
      return v;
  }

  StrOStream stream(state->getAllocator());
  for (int i = 0; i < n; ++i) {
    state->getArg(i).output(state, &stream, false);
  }
  return state->string(stream.getString(), stream.getLength());
}

static Value s_intern(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_STRING);
  String* string = static_cast<String*>(v.toObject());
  return state->intern(string->c_str());
}

static Value s_stringLength(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_STRING);
  String* str = static_cast<String*>(v.toObject());
  return Value(str->len());
}

static Value s_charAt(State* state) {
  Value v = state->getArg(0);
  state->checkType(v, TT_STRING);
  Value i = state->getArg(1);
  state->checkType(i, TT_FIXNUM);
  Fixnum index = i.toFixnum();
  String* str = static_cast<String*>(v.toObject());
  if (index < 0 || static_cast<size_t>(index) >= str->len())
    return Value::NIL;
  return state->character(str->c_str()[i.toFixnum()]);
}

static Value s_substr(State* state) {
  Value sstr = state->getArg(0);
  state->checkType(sstr, TT_STRING);
  String* str = static_cast<String*>(sstr.toObject());
  int len = str->len();
  Value ss = state->getArg(1);
  state->checkType(ss, TT_FIXNUM);
  int s = ss.toFixnum();
  int n = len;
  if (state->getArgNum() > 2) {
    Value nn = state->getArg(2);
    state->checkType(nn, TT_FIXNUM);
    n = nn.toFixnum();
  }

  if (s > len)
    s = len;
  if (n < 0)
    n = 0;
  if (s + n > len)
    n = len - s;
  return state->string(str->c_str() + s, n);
}

static Value s_split(State* state) {
  Value sstr = state->getArg(0);
  state->checkType(sstr, TT_STRING);
  String* str = static_cast<String*>(sstr.toObject());
  Value sd = state->getArg(1);
  state->checkType(sd, TT_STRING);
  String* splitter = static_cast<String*>(sd.toObject());
  int count = 1 << (sizeof(int) * 8 - 2);
  if (state->getArgNum() > 2) {
    Value c = state->getArg(2);
    state->checkType(c, TT_FIXNUM);
    count = c.toFixnum();
  }

  Value result = Value::NIL;
  const char* p = str->c_str();
  const char* d = splitter->c_str();
  int len = splitter->len();
  for (int i = 1; i < count; ++i) {
    const char* q = strstr(p, d);
    if (q == NULL)
      break;
    Value s = state->string(p, q - p);
    result = state->cons(s, result);
    p = q + len;
  }
  if (*p != '\0') {
    if (result.eq(Value::NIL))
      return list(state, sstr);
    Value s = state->string(p);
    result = state->cons(s, result);
  }
  return nreverse(result);
}

static Value s_join(State* state) {
  Value ls = state->getArg(0);
  Value c = state->getArg(1);

  StrOStream stream(state->getAllocator());
  bool first = true;
  for (; !ls.eq(Value::NIL); ls = cdr(ls)) {
    if (!first)
      c.output(state, &stream, false);
    first = false;
    car(ls).output(state, &stream, false);
  }
  return state->string(stream.getString(), stream.getLength());
}

void installBasicFunctions(State* state) {
  state->defineGlobal(Value::NIL, Value::NIL);
  state->defineGlobal(state->getConstant(State::T), state->getConstant(State::T));

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  struct {
    const char* name;
    NativeFuncType func;
    int minArgNum, maxArgNum;
  } static const FuncTable[] = {
    { "macroexpand-1", s_macroexpand_1, 1 },
    { "global-variable-table", s_globalVariableTable, 0 },
    { "type", s_type, 1 },
    { "cons", s_cons, 2 },
    { "car", s_car, 1 },
    { "cdr", s_cdr, 1 },
    { "set-car!", s_setCar, 2 },
    { "set-cdr!", s_setCdr, 2 },
    { "list", s_list, 0, -1 },
    { "list*", s_listStar, 0, -1 },
    { "append", s_append, 0, -1 },
    { "append!", s_appendBang, 0, -1 },
    { "reverse!", s_reverseBang, 1 },
    { "+", s_add, 0, -1 },
    { "-", s_sub, 0, -1 },
    { "*", s_mul, 0, -1 },
    { "/", s_div, 0, -1 },
    { "mod", s_mod, 2 },

    { "eq?", s_eq, 2 },
    { "equal?", s_equal, 2 },
    { "<", s_lessThan, 2, -1 },
    { ">", s_greaterThan, 2, -1 },
    { "<=", s_lessEqual, 2, -1 },
    { ">=", s_greaterEqual, 2, -1 },

    { "ash", s_ash, 2 },
    { "logand", s_logand, 0, -1 },
    { "logior", s_logior, 0, -1 },
    { "logxor", s_logxor, 0, -1 },

    { "write", s_write, 1, 2 },
    { "display", s_display, 1, 2 },
    { "print", s_print, 1, 2 },
    { "format", s_format, 2, -1 },

    { "read", s_read, 0, 2 },
    { "read-from-string", s_readFromString, 1 },
    { "read-delimited-list", s_readDelimitedList, 2 },
    { "read-char", s_readChar, 0, 1 },
    { "unread-char", s_unreadChar, 1, 2 },
    { "read-line", s_readLine, 0, 1 },

    { "gensym", s_gensym, 0 },
    { "uniq", s_gensym, 0 },
    { "apply", s_apply, 1, -1 },
    { "run-binary", s_runBinary, 1 },
    { "load", s_load, 1 },
    { "load-binary", s_loadBinary, 1 },
    { "error", s_error, 1, -1 },

    { "table", s_table, 0, 1 },
    { "table-get", s_tableGet, 2, 3 },
    { "table-put!", s_tablePut, 3 },
    { "table-exists?", s_tableExists, 2 },
    { "table-delete!", s_tableDelete, 2 },
    { "table-keys", s_tableKeys, 1 },

    { "make-vector", s_makeVector, 1, 2 },
    { "vector", s_vector, 0, -1 },
    { "vector-length", s_vectorLength, 1 },
    { "vector-get", s_vectorGet, 2 },
    { "vector-set!", s_vectorSet, 3 },

    { "set-macro-character", s_setMacroCharacter, 2 },
    { "get-macro-character", s_getMacroCharacter, 1 },

    { "collect-garbage", s_collectGarbage, 0 },
    { "exit", s_exit, 1 },
    { "vmtrace", s_vmtrace, 1 },

    { "open", s_open, 1, 2 },
    { "close", s_close, 1 },

    { "int", s_int, 1 },
    { "intern", s_intern, 1 },
    { "char", s_char, 1 },

    { "string", s_string, 1, -1 },
    { "string-length", s_stringLength, 1 },
    { "char-at", s_charAt, 2 },
    { "substr", s_substr, 2, 3 },
    { "split", s_split, 2, 3 },
    { "join", s_join, 2 },
  };

  for (auto it : FuncTable) {
    int maxArgNum = it.maxArgNum == 0 ? it.minArgNum : it.maxArgNum;
    state->defineNative(it.name, it.func, it.minArgNum, maxArgNum);
  }
}

}  // namespace yalp
