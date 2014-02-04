//=============================================================================
/// YALP - Yet Another List Processor.
/**

Naming convension:

SomeType to Value:   Value someType(SomeType x);
Value to SomeType:   Value toSomeType(Value s);

 */
//=============================================================================

#ifndef _YALP_HH_
#define _YALP_HH_

#include "yalp/config.hh"
#include "yalp/error_code.hh"

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>  // for size_t, NULL
#include <stdio.h>  // for FILE

namespace yalp {

class Allocator;
class Reader;
class SHashTable;
class Object;
class State;
class Stream;
class Symbol;
class SymbolManager;
class Vector;
class Vm;

extern const char bootBinaryData[];

typedef long Fixnum;
static_assert(sizeof(Fixnum) >= sizeof(void*),
              "Fixnum must have enough size to store pointer value");

#ifndef DISABLE_FLONUM
#ifdef USE_FLOAT
typedef double Flonum;
#else
typedef float Flonum;
#endif
#endif

typedef void* (*AllocFunc)(void* p, size_t size);

enum Type {
  TT_UNKNOWN,
  TT_FIXNUM,
  TT_SYMBOL,
  TT_CELL,
  TT_STRING,
  TT_CHAR,
#ifndef DISABLE_FLONUM
  TT_FLONUM,  // Floating point number
#endif
  TT_CLOSURE,
  TT_NATIVEFUNC,
  TT_CONTINUATION,
  TT_VECTOR,
  TT_HASH_TABLE,
  TT_STREAM,
  TT_MACRO,
  TT_BOX,  // TODO: This label should not be public, so hide this.
  NUMBER_OF_TYPES,
};

// Variant type.
class Value {
public:
  inline Value();
  inline explicit Value(Fixnum i);
  inline explicit Value(Object* object);
  explicit Value(Fixnum i, int tag2);

  // Gets value type.
  Type getType() const;

  inline bool isFixnum() const;
  inline Fixnum toFixnum() const;
#ifndef DISABLE_FLONUM
  Flonum toFlonum(State* state) const;
#endif
  inline bool isObject() const;
  inline Object* toObject() const;
  const Symbol* toSymbol(State* state) const;
  int toCharacter() const;

  // Object euality.
  inline bool eq(Value target) const;
  bool equal(Value target) const;

  void output(State* state, Stream* o, bool inspect) const;

  unsigned int calcHash(State* state) const;

  void mark();

  static const Value NIL;

  inline bool isTrue() const;
  inline bool isFalse() const;

private:
  void outputSymbol(State* state, Stream* o) const;
  void outputCharacter(Stream* o, bool inspect) const;

  Fixnum v_;
};

typedef Value (*NativeFuncType)(State* state);

// State class.
class State {
public:
  static State* create();
  static State* create(AllocFunc allocFunc);
  // Delete.
  void release();

  ErrorCode runFromFile(const char* filename, Value* pResult = NULL);
  ErrorCode runFromString(const char* string, Value* pResult = NULL);
  ErrorCode runBinaryFromFile(const char* filename, Value* pResult = NULL);
  ErrorCode runBinaryFromString(const char* string, Value* pResult = NULL);

  bool funcall(Value fn, int argNum, const Value* args, Value* pResult);
  Value tailcall(Value fn, int argNum, const Value* args);
  Value applyFunction();

  inline Value referGlobal(const char* sym, bool* pExist = NULL);
  inline void defineGlobal(const char* sym, Value value);
  inline void defineNative(const char* name, NativeFuncType func, int minArgNum);
  Value referGlobal(Value sym, bool* pExist = NULL);
  void defineGlobal(Value sym, Value value);
  void defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);
  SHashTable* getGlobalVariableTable() const;

  // Converts C++ bool value to lisp bool value.
  inline Value boolean(bool b) const;

  // Returns symbol value.
  Value intern(const char* name);
  // Generate unique symbol.
  Value gensym();
  const Symbol* getSymbol(int symbolId) const;

  // Creates cell.
  Value cons(Value a, Value d);

  // Converts character code to lisp character value.
  Value character(int c) const;

  // Converts C string to lisp String.
  Value string(const char* str);
  Value string(const char* str, size_t len);
  Value allocatedString(const char* string, size_t len);  // string is passed.

#ifndef DISABLE_FLONUM
  // Floating point number.
  Value flonum(Flonum f);
#endif

  SHashTable* createHashTable(bool equal);

  // File stream.
  Value createFileStream(FILE* fp);

  Object* getFunc() const;
  // Gets argument number for current native function.
  int getArgNum() const;
  // Gets argument value for the index.
  Value getArg(int index) const;

  // Set multiple values to return values from C function.
  Value multiValues() const;
  Value multiValues(Value v0) const;
  Value multiValues(Value v0, Value v1) const;
  Value multiValues(Value v0, Value v1, Value v2) const;

  // Gets result number.
  int getResultNum() const;
  // Gets result value for the index.
  Value getResult(int index) const;

  int saveArena() const;
  void restoreArena(int index);
  void restoreArenaWith(int index, Value v);

  // Raises runtime error.
  void runtimeError(const char* msg, ...);

  // Constant
  enum Constant {
    T,
    QUOTE,
    QUASIQUOTE,
    UNQUOTE,
    UNQUOTE_SPLICING,
    COMPILE,
    STDIN,
    STDOUT,
    NUMBER_OF_CONSTANTS
  };
  inline Value getConstant(Constant c) const;
  inline Value getTypeSymbol(Type type) const;

  void* alloc(size_t size) const;
  void* realloc(void* ptr, size_t size) const;
  void free(void* ptr) const;
  Allocator* getAllocator()  { return allocator_; }

  void setMacroCharacter(int c, Value func);
  Value getMacroCharacter(int c);
  Value getMacro(Value name);

  // Check value type, and raise runtime error if the value is not expected type.
  void checkType(Value x, Type expected);

  void collectGarbage();
  void setVmTrace(bool b);

  bool compile(Value exp, Value* pValue);

  // Execute compiled code.
  bool runBinary(Value code, Value* pResult);

  void resetError();

  void reportDebugInfo() const;

private:
  struct HashPolicyEq;
  struct HashPolicyEqual;

  State(Allocator* allocator);
  ~State();

  void installBasicObjects();
  ErrorCode run(Stream* stream, Value* pResult);
  ErrorCode runBinary(Stream* stream, Value* pResult);
  void markRoot();
  void allocFailed(void* p, size_t size);

  jmp_buf* setJmpbuf(jmp_buf* jmp);
  void longJmp();

  Allocator* allocator_;
  SymbolManager* symbolManager_;
  Value constants_[NUMBER_OF_CONSTANTS];
  Value typeSymbols_[NUMBER_OF_TYPES];
  HashPolicyEq* hashPolicyEq_;
  HashPolicyEqual* hashPolicyEqual_;
  SHashTable* readTable_;
  Vm* vm_;
  jmp_buf* jmp_;
  int gensymIndex_;

  friend struct StateAllocatorCallback;
};

inline bool Value::eq(Value target) const  { return v_ == target.v_; }
inline bool Value::isTrue() const  { return !eq(Value::NIL); }
inline bool Value::isFalse() const  { return eq(Value::NIL); }

inline Value State::getConstant(State::Constant c) const  { return constants_[c]; }
inline Value State::getTypeSymbol(Type type) const  { return typeSymbols_[type]; }

inline Value State::boolean(bool b) const  { return b ? getConstant(T) : Value::NIL; }

inline Value State::referGlobal(const char* sym, bool* pExist)  { return referGlobal(intern(sym), pExist); }
inline void State::defineGlobal(const char* sym, Value value)  { return defineGlobal(intern(sym), value); }
inline void State::defineNative(const char* name, NativeFuncType func, int minArgNum)  { defineNative(name, func, minArgNum, minArgNum); }

//=============================================================================
/*
  Value: tagged pointer representation.
    XXXXXXX0 : Fixnum
    XXXXXX01 : Object
    XXXX0011 : Symbol
 */

const Fixnum TAG_SHIFT = 2;
const Fixnum TAG_MASK = (1 << TAG_SHIFT) - 1;
const Fixnum TAG_FIXNUM = 0;
const Fixnum TAG_OBJECT = 1;
const Fixnum TAG_OTHER = 3;

Value::Value() : v_(TAG_OBJECT) {
  // Initialized to illegal value.
}

Value::Value(Fixnum i)
  : v_((i << 1) | TAG_FIXNUM) {}

Value::Value(class Object* object)
  : v_(reinterpret_cast<Fixnum>(object) | TAG_OBJECT) {}

bool Value::isFixnum() const  { return (v_ & 1) == TAG_FIXNUM; }

Fixnum Value::toFixnum() const {
  assert(isFixnum());
  return v_ >> 1;
}

bool Value::isObject() const {
  return (v_ & TAG_MASK) == TAG_OBJECT;
}

Object* Value::toObject() const {
  assert(isObject());
  return reinterpret_cast<Object*>(v_ & ~TAG_OBJECT);
}

}  // namespace yalp

#endif
