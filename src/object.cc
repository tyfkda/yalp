//=============================================================================
/// Object
//=============================================================================

#include "yalp/object.hh"
#include "yalp/mem.hh"
#include "hash_table.hh"

namespace yalp {

//=============================================================================
bool Sobject::equal(const Sobject* o) const {
  return this == o;  // Simple pointer equality.
}

unsigned int Sobject::calcHash() const {
  return (reinterpret_cast<long>(this) >> 4) * 23;
}

//=============================================================================
Symbol::Symbol(const char* name)
  : Sobject(), name_(name), hash_(strHash(name)) {}

Type Symbol::getType() const  { return TT_SYMBOL; }

unsigned int Symbol::calcHash() const {
  return hash_;
}

void Symbol::output(State*, std::ostream& o, bool) const {
  o << name_;
}

//=============================================================================
Cell::Cell(Svalue a, Svalue d)
  : Sobject(), car_(a), cdr_(d) {}

Type Cell::getType() const  { return TT_CELL; }

bool Cell::equal(const Sobject* target) const {
  const Cell* p = static_cast<const Cell*>(target);
  return car_.equal(p->car_) && cdr_.equal(p->cdr_);
}

void Cell::output(State* state, std::ostream& o, bool inspect) const {
  const char* abbrev = isAbbrev(state);
  if (abbrev != NULL) {
    o << abbrev;
    state->car(cdr()).output(state, o, inspect);
    return;
  }

  char c = '(';
  const Cell* p;
  for (p = this; ; p = static_cast<Cell*>(p->cdr_.toObject())) {
    o << c;
    p->car_.output(state, o, inspect);
    if (p->cdr_.getType() != TT_CELL)
      break;
    c = ' ';
  }
  if (!p->cdr_.eq(state->nil())) {
    o << " . ";
    p->cdr_.output(state, o, inspect);
  }
  o << ')';
}

void Cell::setCar(Svalue a) {
  car_ = a;
}

void Cell::setCdr(Svalue d) {
  cdr_ = d;
}

const char* Cell::isAbbrev(State* state) const {
  if (car().getType() != TT_SYMBOL ||
      cdr().getType() != TT_CELL ||
      !state->cdr(cdr()).eq(state->nil()))
    return NULL;

  struct {
    State::Constant c;
    const char* abbrev;
  } static const Table[] = {
    { State::QUOTE, "'" },
    { State::QUASIQUOTE, "`" },
    { State::UNQUOTE, "," },
    { State::UNQUOTE_SPLICING, ",@" },
  };
  Svalue a = car();
  for (auto t : Table)
    if (a.eq(state->getConstant(t.c)))
      return t.abbrev;
  return NULL;
}

//=============================================================================

String::String(const char* string)
  : Sobject()
  , string_(string) {
}

Type String::getType() const  { return TT_STRING; }

bool String::equal(const Sobject* target) const {
  const String* p = static_cast<const String*>(target);
  return strcmp(string_, p->string_) == 0;
}

unsigned int String::calcHash() const {
  return strHash(string_);
}

void String::output(State*, std::ostream& o, bool inspect) const {
  if (!inspect) {
    o << string_;
    return;
  }

  o << '"';
  for (const char* p = string_; *p != '\0'; ) {
    char c = *p++;
    switch (c) {
    case '\n':
      o << "\\n";
      break;
    case '\r':
      o << "\\r";
      break;
    case '\t':
      o << "\\t";
      break;
    case '\\':
      o << "\\\\";
      break;
    case '"':
      o << "\\\"";
      break;
    default:
      o << c;
      break;
    }
  }
  o << '"';
}

//=============================================================================

Float::Float(Sfloat v)
  : Sobject()
  , v_(v) {
}

Type Float::getType() const  { return TT_FLOAT; }

bool Float::equal(const Sobject* target) const {
  const Float* p = static_cast<const Float*>(target);
  return v_ == p->v_;
}

void Float::output(State*, std::ostream& o, bool) const {
  o << v_;
}

//=============================================================================

struct SHashTable::Policy : public SHashTable::TableType::Policy {
  virtual unsigned int hash(const Svalue a) override  { return a.calcHash(); }
  virtual bool equal(const Svalue a, const Svalue b) override  { return a.eq(b); }
};

SHashTable::Policy SHashTable::s_policy;

SHashTable::SHashTable(Allocator* allocator)
  : Sobject() {
  void* memory = allocator->alloc(sizeof(*table_));
  table_ = new(memory) SHashTable::TableType(&s_policy);
  // TODO: Ensure table_ is released on destructor.
}

Type SHashTable::getType() const  { return TT_HASH_TABLE; }

void SHashTable::output(State*, std::ostream& o, bool) const {
  o << "#<hash-table:" << this << ">";
}

int SHashTable::getCapacity() const  { return table_->getCapacity(); }
int SHashTable::getEntryCount() const  { return table_->getEntryCount(); }
int SHashTable::getConflictCount() const  { return table_->getConflictCount(); }
int SHashTable::getMaxDepth() const  { return table_->getMaxDepth(); }

void SHashTable::put(Svalue key, Svalue value) {
  table_->put(key, value);
}

const Svalue* SHashTable::get(Svalue key) const {
  return table_->get(key);
}

bool SHashTable::remove(Svalue key) {
  return table_->remove(key);
}

//=============================================================================

}  // namespace yalp
