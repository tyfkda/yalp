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

#include "yalp/error_code.hh"

#include <setjmp.h>
#include <stddef.h>  // for size_t, NULL
#include <stdio.h>  // for FILE

namespace yalp {

class Allocator;
class Reader;
class SHashTable;
class Sobject;
class State;
class Stream;
class Symbol;
class SymbolManager;
class Vm;

// This must be able to hold native pointer size value.
typedef long Fixnum;
typedef double Flonum;

typedef void* (*AllocFunc)(void* p, size_t size);

enum Type {
  TT_UNKNOWN,
  TT_FIXNUM,
  TT_SYMBOL,
  TT_CELL,
  TT_STRING,
  TT_FLONUM,  // Floating point number
  TT_CLOSURE,
  TT_NATIVEFUNC,
  TT_CONTINUATION,
  TT_VECTOR,
  TT_HASH_TABLE,
  TT_STREAM,
  TT_BOX,  // TODO: This label should not be public, so hide this.
  NUMBER_OF_TYPES,
};

// Variant type.
class Value {
public:
  Value();

  // Gets value type.
  Type getType() const;

  Fixnum toFixnum() const;
  Flonum toFlonum(State* state) const;
  bool isObject() const;
  Sobject* toObject() const;
  const Symbol* toSymbol(State* state) const;
  int toCharacter() const  { return toFixnum(); }

  // Object euality.
  bool eq(Value target) const  { return v_ == target.v_; }
  bool equal(Value target) const;

  void output(State* state, Stream* o, bool inspect) const;

  long getId() const  { return v_; }
  unsigned int calcHash(State* state) const;

  void mark();

  static const Value NIL;

  inline bool isTrue() const;
  inline bool isFalse() const;

private:
  explicit Value(Fixnum i);
  explicit Value(Sobject* object);
  explicit Value(Fixnum i, int tag2);

  Fixnum v_;

  friend Reader;
  friend State;
  friend Vm;
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

  bool funcall(Value fn, int argNum, const Value* args, Value* pResult);
  Value tailcall(Value fn, int argNum, const Value* args);

  inline Value referGlobal(const char* sym, bool* pExist = NULL);
  inline void defineGlobal(const char* sym, Value value);
  inline void defineNative(const char* name, NativeFuncType func, int minArgNum);
  Value referGlobal(Value sym, bool* pExist = NULL);
  void defineGlobal(Value sym, Value value);
  void defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);

  // Converts C++ bool value to lisp bool value.
  inline Value boolean(bool b) const;

  // Converts C++ int value to lisp Fixnum.
  inline Value fixnum(Fixnum i) const;

  // Returns symbol value.
  Value intern(const char* name);
  Value gensym();
  const Symbol* getSymbol(unsigned int symbolId) const;

  // Creates cell.
  Value cons(Value a, Value d);
  Value car(Value s);
  Value cdr(Value s);

  // Converts character code to lisp character value.
  inline Value character(int c) const;

  // Converts C string to lisp String.
  Value string(const char* str);
  Value string(const char* str, int len);
  Value allocatedString(const char* string, int len);  // string is passed.

  // Floating point number.
  Value flonum(Flonum f);

  Value createHashTable();

  // File stream.
  Value createFileStream(FILE* fp);

  // Gets argument number for current native function.
  int getArgNum() const;
  // Gets argument value for the index.
  Value getArg(int index) const;

  // Gets result number.
  int getResultNum() const;
  // Gets result value for the index.
  Value getResult(int index) const;

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
    NUMBER_OF_CONSTANTS
  };
  inline Value getConstant(Constant c) const;
  inline Value getTypeSymbol(Type type) const;

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

  State(AllocFunc allocFunc);
  ~State();

  void installBasicObjects();
  ErrorCode run(Stream* stream, Value* pResult);
  void markRoot();
  void allocFailed(void* p, size_t size);

  jmp_buf* setJmpbuf(jmp_buf* jmp);
  void longJmp();

  AllocFunc allocFunc_;
  Allocator* allocator_;
  SymbolManager* symbolManager_;
  Value constants_[NUMBER_OF_CONSTANTS];
  Value typeSymbols_[NUMBER_OF_TYPES];
  HashPolicyEq* hashPolicyEq_;
  SHashTable* readTable_;
  Vm* vm_;
  jmp_buf* jmp_;

  friend struct StateAllocatorCallback;
};

inline bool Value::isTrue() const  { return !eq(Value::NIL); }
inline bool Value::isFalse() const  { return eq(Value::NIL); }

inline Value State::getConstant(State::Constant c) const  { return constants_[c]; }
inline Value State::getTypeSymbol(Type type) const  { return typeSymbols_[type]; }

inline Value State::boolean(bool b) const  { return b ? getConstant(T) : Value::NIL; }
inline Value State::fixnum(Fixnum i) const  { return Value(i); }
inline Value State::character(int c) const  { return Value(c); }  // Use fixnum as a character.

inline Value State::referGlobal(const char* sym, bool* pExist)  { return referGlobal(intern(sym), pExist); }
inline void State::defineGlobal(const char* sym, Value value)  { return defineGlobal(intern(sym), value); }
inline void State::defineNative(const char* name, NativeFuncType func, int minArgNum)  { defineNative(name, func, minArgNum, minArgNum); }

}  // namespace yalp

#endif
