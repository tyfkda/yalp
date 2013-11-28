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

#include <ostream>

namespace yalp {

class Allocator;
class State;
class SymbolManager;
class Vm;

// This must be able to hold native pointer size value.
typedef long Sfixnum;
typedef float Sfloat;

enum Type {
  TT_UNKNOWN,
  TT_FIXNUM,
  TT_SYMBOL,
  TT_CELL,
  TT_STRING,
  TT_FLOAT,
  TT_CLOSURE,
  TT_NATIVEFUNC,
  TT_BOX,
  TT_VECTOR,
  TT_HASH_TABLE,
};

// S-value: bit embedded type value.
class Svalue {
public:
  Svalue();

  // Gets value type.
  Type getType() const;

  Sfixnum toFixnum() const;
  Sfloat toFloat() const;
  class Sobject* toObject() const;

  // Object euality.
  bool eq(Svalue target) const  { return v_ == target.v_; }
  bool equal(Svalue target) const;

  void output(State* state, std::ostream& o, bool inspect) const;

  long getId() const  { return v_; }
  unsigned int calcHash() const;

private:
  explicit Svalue(Sfixnum i);
  explicit Svalue(class Sobject* object);

  Sfixnum v_;

  friend State;
  friend class Vm;
};

typedef Svalue (*NativeFuncType)(State* state);

// State class.
class State {
public:
  static State* create();
  static State* create(Allocator* allocator);
  // Delete.
  void release();

  bool compile(Svalue exp, Svalue* pValue);

  // Execute compiled code.
  Svalue runBinary(Svalue code);

  bool runFromFile(const char* filename, Svalue* pResult = NULL);
  bool runBinaryFromFile(const char* filename, Svalue* pResult = NULL);

  Svalue nil() const  { return nil_; }
  Svalue t() const  { return t_; }
  bool isTrue(Svalue x) const  { return !x.eq(nil_); }
  bool isFalse(Svalue x) const  { return x.eq(nil_); }

  // Converts C++ bool value to lisp bool value.
  Svalue boolValue(bool b) const  { return b ? t() : nil(); }

  // Converts C++ int value to lisp Fixnum.
  Svalue fixnumValue(Sfixnum i)  { return Svalue(i); }

  // Returns symbol value.
  Svalue intern(const char* name);
  Svalue gensym();

  // Creates cell.
  Svalue cons(Svalue a, Svalue d);
  Svalue car(Svalue s);
  Svalue cdr(Svalue s);

  // Converts C string to lisp String.
  Svalue stringValue(const char* string);

  // Floating point number.
  Svalue floatValue(Sfloat f);

  // Gets argument number for current native function.
  int getArgNum() const;
  // Gets argument value for the index.
  Svalue getArg(int index) const;

  // Raises runtime error.
  void runtimeError(const char* msg);

  // Wrap a value with quote.
  Svalue quote(Svalue x);

  Svalue createHashTable();

  Allocator* getAllocator() const  { return allocator_; }

  Svalue referGlobal(Svalue sym, bool* pExist = NULL);
  void assignGlobal(Svalue sym, Svalue value);
  void assignNative(const char* name, NativeFuncType func, int minArgNum) {
    assignNative(name, func, minArgNum, minArgNum);
  }
  void assignNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);

  Svalue funcall(Svalue fn, int argNum, const Svalue* args);

  void reportDebugInfo() const;

private:
  State(Allocator* allocator);
  ~State();

  Allocator* allocator_;
  SymbolManager* symbolManager_;
  Svalue nil_;
  Svalue t_;
  Svalue quote_;
  Vm* vm_;
};

// Helper functions.
Svalue list(State* state, Svalue v1);
Svalue list(State* state, Svalue v1, Svalue v2);
Svalue list(State* state, Svalue v1, Svalue v2, Svalue v3);
Svalue nreverse(State* state, Svalue v);
int length(State* state, Svalue v);

}  // namespace yalp

#endif
