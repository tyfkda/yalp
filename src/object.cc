//=============================================================================
/// object
//=============================================================================

#include "yalp/object.hh"

namespace yalp {

//=============================================================================
bool Sobject::equal(const Sobject* o) const {
  return this == o;  // Simple pointer equality.
}

//=============================================================================
Symbol::Symbol(const char* name)
  : Sobject(), name_(name) {}

Type Symbol::getType() const  { return TT_SYMBOL; }

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

void Cell::rplaca(Svalue a) {
  car_ = a;
}

void Cell::rplacd(Svalue d) {
  cdr_ = d;
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

HashTable::HashTable()
  : Sobject()
  , table_() {
}

Type HashTable::getType() const  { return TT_HASH_TABLE; }

void HashTable::output(State*, std::ostream& o, bool) const {
  o << "#<hash-table:" << this << ">";
}

void HashTable::put(Svalue key, Svalue value) {
  table_[key.calcHash()] = value;
}

bool HashTable::get(Svalue key, Svalue* pValue) const {
  auto it = table_.find(key.calcHash());
  if (it == table_.end())
    return false;

  *pValue = it->second;
  return true;
}

bool HashTable::erase(Svalue key) {
  auto it = table_.find(key.calcHash());
  if (it == table_.end())
    return false;

  table_.erase(it);
  return true;
}

//=============================================================================

}  // namespace yalp
