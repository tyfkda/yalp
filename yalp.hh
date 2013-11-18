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

class State;
class SymbolManager;
class Vm;

// This must be able to hold native pointer size value.
typedef long Sfixnum;

enum Type {
  TT_UNKNOWN,
  TT_FIXNUM,
  TT_SYMBOL,
  TT_CELL,
  TT_STRING,
  TT_CLOSURE,
  TT_NATIVEFUNC,
  TT_BOX,
  TT_VECTOR,
};

// S-value: bit embedded type value.
class Svalue {
public:
  Svalue();

  // Gets value type.
  Type getType() const;

  Sfixnum toFixnum() const;
  class Sobject* toObject() const;

  // Object euality.
  bool eq(Svalue target) const  { return v_ == target.v_; }
  bool equal(Svalue target) const;

  friend std::ostream& operator<<(std::ostream& o, Svalue v);

  long getHash() const  { return v_; }

private:
  explicit Svalue(Sfixnum i);
  explicit Svalue(class Sobject* object);

  Sfixnum v_;

  friend State;
  friend class Vm;
};

// State class.
class State {
public:
  static State* create();
  virtual ~State();

  Svalue runBinary(Svalue code);

  Svalue nil() const  { return nil_; }
  Svalue t() const  { return t_; }
  bool isTrue(Svalue x) const  { return !x.eq(nil_); }
  bool isFalse(Svalue x) const  { return x.eq(nil_); }
  Svalue boolValue(bool b) const  { return b ? t() : nil(); }

  // Fixnum.
  Svalue fixnumValue(Sfixnum i)  { return Svalue(i); }

  // Create symbol.
  Svalue intern(const char* name);

  // Create cell.
  Svalue cons(Svalue a, Svalue d);

  // String.
  Svalue stringValue(const char* string);

  int getArgNum() const;
  Svalue getArg(int index) const;
  void runtimeError(const char* msg);

  Svalue quote(Svalue x);

private:
  State();

  SymbolManager* symbolManager_;
  Svalue nil_;
  Svalue t_;
  Svalue quote_;
  Vm* vm_;
};

// Helper functions.
Svalue list1(State* state, Svalue v1);
Svalue list2(State* state, Svalue v1, Svalue v2);
Svalue list3(State* state, Svalue v1, Svalue v2, Svalue v3);
Svalue nreverse(State* state, Svalue v);

}  // namespace yalp

#endif
