//=============================================================================
/// MACP - Macro Processor.
//=============================================================================

#include "macp.hh"
#include <assert.h>
#include <stdlib.h>

namespace macp {

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

Type State::getType(Svalue v) const {
  switch (v & TAG_MASK) {
  default:
    assert(false);
    break;
  case TAG_FIXNUM:
    return TT_FIXNUM;
  case TAG_OBJECT:
    return toObject(v)->getType();
  }
}

Svalue State::fixnumValue(Sfixnum i) {
  return reinterpret_cast<Svalue>(i) | TAG_FIXNUM;
}

Sfixnum State::toFixnum(Svalue v) {
  assert((v & TAG_MASK) == TAG_FIXNUM);
  assert(TAG_FIXNUM == 0);
  return reinterpret_cast<Sfixnum>(v);
}

Svalue State::cons(Svalue a, Svalue d) {
  Scell* cell = new Scell(a, d);
  return objectValue(cell);
}

Svalue State::objectValue(Sobject* o) {
  return reinterpret_cast<Svalue>(o) | TAG_OBJECT;
}

Sobject* State::toObject(Svalue v) const {
  assert((v & TAG_MASK) == TAG_OBJECT);
  return reinterpret_cast<Sobject*>(v & ~TAG_OBJECT);
}


//=============================================================================
Sobject::~Sobject() {
}

//=============================================================================
Scell::Scell(Svalue a, Svalue d) : car_(a), cdr_(d) {
}

//=============================================================================

}  // namespace macp
