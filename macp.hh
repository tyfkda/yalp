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

// S-value: bit embedded type value.
// This must be able to hold native pointer size value.
typedef long Svalue;

typedef Svalue Sfixnum;

const Svalue TAG_SHIFT = 2;
const Svalue TAG_MASK = (1 << TAG_SHIFT) - 1;
const Svalue TAG_FIXNUM = 0;
const Svalue TAG_OBJECT = 1;

enum Type {
  TT_UNKNOWN,
  TT_FIXNUM,
  TT_CELL,
};

// State class.
class State {
public:
  static State* create();
  virtual ~State();

  Svalue readString(const char* str);

  // Gets value type.
  Type getType(Svalue v) const;

  // Fixnum.
  Svalue fixnumValue(Sfixnum i);
  Sfixnum toFixnum(Svalue v);

  // Create cell.
  Svalue cons(Svalue a, Svalue d);

  // Object.
  Svalue objectValue(class Sobject* o);
  Sobject* toObject(Svalue v) const;

  // Object euality.
  static bool eq(Svalue a, Svalue b)  { return a == b; }

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
