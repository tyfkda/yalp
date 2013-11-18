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

  friend class State;
  friend class SymbolManager;
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

  Svalue quote(Svalue x);

  // Object.
  Svalue objectValue(class Sobject* o)  { return Svalue(o); }

  // String.
  Svalue stringValue(const char* string);

  int getArgNum() const;
  Svalue getArg(int index) const;
  void runtimeError(const char* msg);

private:
  State();

  SymbolManager* symbolManager_;
  Svalue nil_;
  Svalue t_;
  Svalue quote_;
  Vm* vm_;
};


// Base class.
class Sobject {
public:
  virtual ~Sobject();
  virtual Type getType() const = 0;
  virtual bool equal(const Sobject* target) const;

  virtual std::ostream& operator<<(std::ostream& o) const = 0;
};

inline std::ostream& operator<<(std::ostream& o, Sobject& object) {
  return object.operator<<(o);
}

// Symbol class.
class Symbol : public Sobject {
public:
  virtual Type getType() const override;

  const char* c_str() const  { return name_; }

  virtual std::ostream& operator<<(std::ostream& o) const override;

protected:
  Symbol(const char* name);
  const char* name_;

  friend class SymbolManager;
};

// Cell class.
class Cell : public Sobject {
public:
  virtual Type getType() const override;
  virtual bool equal(const Sobject* target) const override;

  Svalue car() const  { return car_; }
  Svalue cdr() const  { return cdr_; }
  void rplaca(Svalue a);
  void rplacd(Svalue d);

  virtual std::ostream& operator<<(std::ostream& o) const override;

protected:
  Cell(Svalue a, Svalue d);
  Svalue car_;
  Svalue cdr_;

  friend State;
};

// String class.
class String : public Sobject {
public:
  virtual Type getType() const override;
  virtual bool equal(const Sobject* target) const override;

  virtual std::ostream& operator<<(std::ostream& o) const override;

protected:
  String(const char* string);
  const char* string_;

  friend State;
};


// Helper functions.
Svalue list1(State* state, Svalue v1);
Svalue list2(State* state, Svalue v1, Svalue v2);
Svalue list3(State* state, Svalue v1, Svalue v2, Svalue v3);
Svalue nreverse(State* state, Svalue v);

}  // namespace yalp

#endif
