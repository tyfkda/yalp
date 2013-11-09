//=============================================================================
/// MACP - Macro Processor.
//=============================================================================

#include "macp.hh"
#include <assert.h>
#include <stdlib.h>

namespace macp {
//=============================================================================
Svalue::Svalue(Sfixnum i)
  : v_(reinterpret_cast<Sfixnum>(i << TAG_SHIFT) | TAG_FIXNUM) {}

Svalue::Svalue(class Sobject* object)
  : v_(reinterpret_cast<Sfixnum>(object) | TAG_OBJECT) {}

Type Svalue::getType() const {
  switch (v_ & TAG_MASK) {
  default:
    assert(false);
    return TT_UNKNOWN;
  case TAG_FIXNUM:
    return TT_FIXNUM;
  case TAG_OBJECT:
    return toObject()->getType();
  }
}

Sfixnum Svalue::toFixnum() const {
  assert((v_ & TAG_MASK) == TAG_FIXNUM);
  assert(TAG_FIXNUM == 0);
  return reinterpret_cast<Sfixnum>(v_ >> TAG_SHIFT);
}

Sobject* Svalue::toObject() const {
  assert((v_ & TAG_MASK) == TAG_OBJECT);
  return reinterpret_cast<Sobject*>(v_ & ~TAG_OBJECT);
}

bool Svalue::equal(Svalue target) const {
  switch (v_ & TAG_MASK) {
  default:
    assert(false);
    return false;
  case TAG_FIXNUM:
    return eq(target);
  case TAG_OBJECT:
    {
      Type t1 = getType();
      Type t2 = target.getType();
      if (t1 != t2)
        return false;

      return toObject()->equal(target.toObject());
    }
  }
}

//=============================================================================
State* State::create() {
  return new State;
}

State::State() {
}

State::~State() {
}

Svalue State::readString(const char* str) {
  return fixnumValue(atoi(str));
}

Svalue State::cons(Svalue a, Svalue d) {
  Scell* cell = new Scell(a, d);
  return Svalue(cell);
}


//=============================================================================
Sobject::~Sobject() {
}

//=============================================================================
Scell::Scell(Svalue a, Svalue d) : car_(a), cdr_(d) {
}

Type Scell::getType() const  { return TT_CELL; }

bool Scell::equal(const Sobject* target) const {
  const Scell* p = static_cast<const Scell*>(target);
  return car_.equal(p->car_) && cdr_.equal(p->cdr_);
}

//=============================================================================

}  // namespace macp
