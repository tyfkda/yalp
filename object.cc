//=============================================================================
/// object
//=============================================================================

#include "object.hh"

namespace yalp {

//=============================================================================
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
  char c = '(';
  const Cell* p;
  for (p = this; ; p = static_cast<Cell*>(p->cdr_.toObject())) {
    o << c << p->car_;
    if (p->cdr_.getType() != TT_CELL)
      break;
    c = ' ';
  }
  return o << " . " << p->cdr_ << ')';
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

std::ostream& String::operator<<(std::ostream& o) const {
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
  return o << '"';
}

//=============================================================================

}  // namespace yalp
