//=============================================================================
/// MACP - Macro Processor.
//=============================================================================

#include "macp.hh"
#include "symbol_manager.hh"
#include <assert.h>

namespace macp {

const Sfixnum TAG_SHIFT = 2;
const Sfixnum TAG_MASK = (1 << TAG_SHIFT) - 1;
const Sfixnum TAG_FIXNUM = 0;
const Sfixnum TAG_OBJECT = 1;

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

std::ostream& operator<<(std::ostream& o, Svalue v) {
  switch (v.v_ & TAG_MASK) {
  default:
    assert(false);
    return o;
  case TAG_FIXNUM:
    o << v.toFixnum();
    return o;
  case TAG_OBJECT:
    return o << *v.toObject();
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

State::State()
  : symbolManager_(new SymbolManager()),
    nil_(symbolManager_->intern("nil")) {
}

State::~State() {
  delete symbolManager_;
}

Svalue State::intern(const char* name) {
  return symbolManager_->intern(name);
}

Svalue State::cons(Svalue a, Svalue d) {
  Cell* cell = new Cell(a, d);
  return Svalue(cell);
}


//=============================================================================
Sobject::~Sobject() {
}

bool Sobject::equal(const Sobject* o) const {
  return this == o;  // Simple pointer equality.
}

//=============================================================================
Symbol::Symbol(const char* name)
  : Sobject(), name_(name) {}

Type Symbol::getType() const  { return TT_SYMBOL; }

std::ostream& Symbol::operator<<(std::ostream& o) const {
  return o << name_;
}

//=============================================================================
Cell::Cell(Svalue a, Svalue d)
  : Sobject(), car_(a), cdr_(d) {}

Type Cell::getType() const  { return TT_CELL; }

bool Cell::equal(const Sobject* target) const {
  const Cell* p = static_cast<const Cell*>(target);
  return car_.equal(p->car_) && cdr_.equal(p->cdr_);
}

std::ostream& Cell::operator<<(std::ostream& o) const {
  return o << '(' << car_ << " . " << cdr_ << ')';
}

void Cell::rplaca(Svalue a) {
  car_ = a;
}

void Cell::rplacd(Svalue d) {
  cdr_ = d;
}

//=============================================================================

Svalue list1(State* state, Svalue v1) {
  return state->cons(v1, state->nil());
}

Svalue list2(State* state, Svalue v1, Svalue v2) {
  return state->cons(v1, state->cons(v2, state->nil()));
}

Svalue list3(State* state, Svalue v1, Svalue v2, Svalue v3) {
  return state->cons(v1, state->cons(v2, state->cons(v3, state->nil())));
}

Svalue nreverse(State* state, Svalue v) {
  if (v.getType() != TT_CELL)
    return v;

  Svalue tail = state->nil();
  for (;;) {
    Cell* cell = static_cast<Cell*>(v.toObject());
    Svalue d = cell->cdr();
    cell->rplacd(tail);
    if (d.getType() != TT_CELL)
      return v;
    tail = v;
    v = d;
  }
}

//=============================================================================

}  // namespace macp
