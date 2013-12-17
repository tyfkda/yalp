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

#include <stddef.h>  // for size_t, NULL
#include <stdio.h>  // for FILE

namespace yalp {

class Allocator;
class Sobject;
class State;
class Stream;
class SStream;
class Symbol;
class SymbolManager;
class Vm;

// This must be able to hold native pointer size value.
typedef long Sfixnum;
typedef double Sfloat;

typedef void* (*AllocFunc)(void* p, size_t size);

enum Type {
  TT_UNKNOWN,
  TT_FIXNUM,
  TT_SYMBOL,
  TT_CELL,
  TT_STRING,
  TT_FLOAT,
  TT_CLOSURE,
  TT_NATIVEFUNC,
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

  Sfixnum toFixnum() const;
  Sfloat toFloat(State* state) const;
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
  explicit Svalue(Sfixnum i);
  explicit Svalue(Sobject* object);
  explicit Svalue(Sfixnum i, int tag2);

  Sfixnum v_;

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

  bool compile(Svalue exp, Svalue* pValue);

  // Execute compiled code.
  bool runBinary(Svalue code, Svalue* pResult);

  bool runFromFile(const char* filename, Svalue* pResult = NULL);
  bool runBinaryFromFile(const char* filename, Svalue* pResult = NULL);

  // Check value type, and raise runtime error if the value is not expected type.
  void checkType(Svalue x, Type expected);

  // Converts C++ bool value to lisp bool value.
  Svalue boolValue(bool b) const  { return b ? getConstant(T) : Svalue::NIL; }

  // Converts C++ int value to lisp Fixnum.
  Svalue fixnumValue(Sfixnum i)  { return Svalue(i); }

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

  // Floating point number.
  Svalue floatValue(Sfloat f);

  // File stream.
  Svalue createFileStream(FILE* fp);

  // Gets argument number for current native function.
  int getArgNum() const;
  // Gets argument value for the index.
  Svalue getArg(int index) const;

  // Raises runtime error.
  void runtimeError(const char* msg);

  // Constant
  enum Constant {
    T,
    QUOTE,
    QUASIQUOTE,
    UNQUOTE,
    UNQUOTE_SPLICING,

    NUMBER_OF_CONSTANT
  };
  Svalue getConstant(Constant c) const  { return constant_[c]; }
  bool isTrue(Svalue x) const  { return !x.eq(Svalue::NIL); }
  bool isFalse(Svalue x) const  { return x.eq(Svalue::NIL); }

  Svalue createHashTable();

  Allocator* getAllocator()  { return allocator_; }

  Svalue referGlobal(Svalue sym, bool* pExist = NULL);
  void defineGlobal(Svalue sym, Svalue value);
  void defineNative(const char* name, NativeFuncType func, int minArgNum) {
    defineNative(name, func, minArgNum, minArgNum);
  }
  void defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);
  Svalue getMacro(Svalue name);

  bool funcall(Svalue fn, int argNum, const Svalue* args, Svalue* pResult);
  Svalue tailcall(Svalue fn, int argNum, const Svalue* args);
  void resetError();

  void collectGarbage();
  void setVmTrace(bool b);

  void reportDebugInfo() const;

private:
  struct HashPolicyEq;

  State(AllocFunc allocFunc);
  ~State();

  void installBasicObjects();
  void markRoot();
  void allocFailed(void* p, size_t size);

  AllocFunc allocFunc_;
  Allocator* allocator_;
  SymbolManager* symbolManager_;
  Svalue constant_[NUMBER_OF_CONSTANT];
  HashPolicyEq* hashPolicyEq_;
  Vm* vm_;

  friend struct StateAllocatorCallback;
};

}  // namespace yalp

#endif
