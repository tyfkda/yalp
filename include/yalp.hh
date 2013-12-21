//=============================================================================
/// YALP - Yet Another List Processor.
/**

Naming convension:

SomeType to Svalue:   Svalue someTypeValue(SomeType x);
Svalue to SomeType:   Svalue toSomeType(Svalue s);

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
class SStream;
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
};

// Variant type.
class Svalue {
public:
  Svalue();

  // Gets value type.
  Type getType() const;

  Fixnum toFixnum() const;
  Flonum toFlonum(State* state) const;
  bool isObject() const;
  Sobject* toObject() const;
  const Symbol* toSymbol(State* state) const;
  int toCharacter() const  { return toFixnum(); }

  // Object euality.
  bool eq(Svalue target) const  { return v_ == target.v_; }
  bool equal(Svalue target) const;

  void output(State* state, Stream* o, bool inspect) const;

  long getId() const  { return v_; }
  unsigned int calcHash(State* state) const;

  void mark();

  static const Svalue NIL;

private:
  explicit Svalue(Fixnum i);
  explicit Svalue(Sobject* object);
  explicit Svalue(Fixnum i, int tag2);

  Fixnum v_;

  friend Reader;
  friend State;
  friend Vm;
};

typedef Svalue (*NativeFuncType)(State* state);

// State class.
class State {
public:
  static State* create();
  static State* create(AllocFunc allocFunc);
  // Delete.
  void release();

  ErrorCode runFromFile(const char* filename, Svalue* pResult = NULL);
  ErrorCode runFromString(const char* string, Svalue* pResult = NULL);
  ErrorCode runBinaryFromFile(const char* filename, Svalue* pResult = NULL);

  bool funcall(Svalue fn, int argNum, const Svalue* args, Svalue* pResult);
  Svalue tailcall(Svalue fn, int argNum, const Svalue* args);

  inline Svalue referGlobal(const char* sym, bool* pExist = NULL);
  inline void defineGlobal(const char* sym, Svalue value);
  Svalue referGlobal(Svalue sym, bool* pExist = NULL);
  void defineGlobal(Svalue sym, Svalue value);
  void defineNative(const char* name, NativeFuncType func, int minArgNum) {
    defineNative(name, func, minArgNum, minArgNum);
  }
  void defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);

  // Converts C++ bool value to lisp bool value.
  Svalue boolValue(bool b) const  { return b ? getConstant(T) : Svalue::NIL; }

  // Converts C++ int value to lisp Fixnum.
  Svalue fixnumValue(Fixnum i)  { return Svalue(i); }

  // Returns symbol value.
  Svalue intern(const char* name);
  Svalue gensym();
  const Symbol* getSymbol(unsigned int symbolId) const;

  // Creates cell.
  Svalue cons(Svalue a, Svalue d);
  Svalue car(Svalue s);
  Svalue cdr(Svalue s);

  // Converts character code to lisp character value.
  Svalue characterValue(int c)  { return Svalue(c); }  // Use fixnum as a character.

  // Converts C string to lisp String.
  Svalue stringValue(const char* string);
  Svalue stringValue(const char* string, int len);
  Svalue allocatedStringValue(const char* string, int len);  // string is passed.

  // Floating point number.
  Svalue flonumValue(Flonum f);

  Svalue createHashTable();

  // File stream.
  Svalue createFileStream(FILE* fp);

  // Gets argument number for current native function.
  int getArgNum() const;
  // Gets argument value for the index.
  Svalue getArg(int index) const;

  // Gets result number.
  int getResultNum() const;
  // Gets result value for the index.
  Svalue getResult(int index) const;

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

    NUMBER_OF_CONSTANT
  };
  Svalue getConstant(Constant c) const  { return constant_[c]; }
  bool isTrue(Svalue x) const  { return !x.eq(Svalue::NIL); }
  bool isFalse(Svalue x) const  { return x.eq(Svalue::NIL); }

  Allocator* getAllocator()  { return allocator_; }

  void setMacroCharacter(int c, Svalue func);
  Svalue getMacroCharacter(int c);
  Svalue getMacro(Svalue name);

  // Check value type, and raise runtime error if the value is not expected type.
  void checkType(Svalue x, Type expected);

  void collectGarbage();
  void setVmTrace(bool b);

  bool compile(Svalue exp, Svalue* pValue);

  // Execute compiled code.
  bool runBinary(Svalue code, Svalue* pResult);

  void resetError();

  void reportDebugInfo() const;

private:
  struct HashPolicyEq;

  State(AllocFunc allocFunc);
  ~State();

  void installBasicObjects();
  ErrorCode run(Stream* stream, Svalue* pResult);
  void markRoot();
  void allocFailed(void* p, size_t size);

  jmp_buf* setJmpbuf(jmp_buf* jmp);
  void longJmp();

  AllocFunc allocFunc_;
  Allocator* allocator_;
  SymbolManager* symbolManager_;
  Svalue constant_[NUMBER_OF_CONSTANT];
  HashPolicyEq* hashPolicyEq_;
  SHashTable* readTable_;
  Vm* vm_;
  jmp_buf* jmp_;

  friend struct StateAllocatorCallback;
};

inline Svalue State::referGlobal(const char* sym, bool* pExist)  { return referGlobal(intern(sym), pExist); }
inline void State::defineGlobal(const char* sym, Svalue value)  { return defineGlobal(intern(sym), value); }

}  // namespace yalp

#endif
