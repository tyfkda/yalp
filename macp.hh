//=============================================================================
/// MACP - Macro Processor.
/**

Naming convension:

SomeType to Svalue:   Svalue someTypeValue(SomeType x);
Svalue to SomeType:   Svalue toSomeType(Svalue s);

 */
//=============================================================================

#ifndef _MACP_HH_
#define _MACP_HH_

namespace macp {

// This must be able to hold native pointer size value.
typedef long Sfixnum;

const Sfixnum TAG_SHIFT = 2;
const Sfixnum TAG_MASK = (1 << TAG_SHIFT) - 1;
const Sfixnum TAG_FIXNUM = 0;
const Sfixnum TAG_OBJECT = 1;

enum Type {
  TT_UNKNOWN,
  TT_FIXNUM,
  TT_CELL,
};

// S-value: bit embedded type value.
class Svalue {
public:
  Svalue(Sfixnum i);
  Svalue(class Sobject* object);

  // Gets value type.
  Type getType() const;

  Sfixnum toFixnum() const;
  class Sobject* toObject() const;

  // Object euality.
  bool eq(Svalue target) const  { return v_ == target.v_; }

  bool operator==(Svalue target) const  { return eq(target); }

private:
  Sfixnum v_;
};

// State class.
class State {
public:
  static State* create();
  virtual ~State();

  Svalue readString(const char* str);

  // Fixnum.
  Svalue fixnumValue(Sfixnum i)  { return Svalue(i); }

  // Create cell.
  Svalue cons(Svalue a, Svalue d);

  // Object.
  Svalue objectValue(class Sobject* o)  { return Svalue(o); }

private:
  State();
};


// Base class.
class Sobject {
public:
  virtual ~Sobject();
  virtual Type getType() const = 0;
};

// Cell class.
class Scell : public Sobject {
public:
  virtual Type getType() const override  { return TT_CELL; }
  Svalue car() const  { return car_; }
  Svalue cdr() const  { return cdr_; }

protected:
  Scell(Svalue a, Svalue d);
  Svalue car_;
  Svalue cdr_;

  friend State;
};

}  // namespace macp

#endif
