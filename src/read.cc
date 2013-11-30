//=============================================================================
/// Read S-Expression
//=============================================================================

#include "yalp/read.hh"
#include "yalp/object.hh"
#include "hash_table.hh"
#include <assert.h>

namespace yalp {

struct Reader::IntHashPolicy : public HashPolicy<int> {
  virtual unsigned int hash(int a) override  { return a; }
  virtual bool equal(int a, int b) override  { return a == b; }
};

Reader::IntHashPolicy Reader::s_hashPolicy;

Reader::Reader(State* state, std::istream& istrm)
  : state_(state), istrm_(istrm)
  , sharedStructures_(NULL) {
}

Reader::~Reader() {
  if (sharedStructures_ != NULL) {
    sharedStructures_->~HashTable<int, Svalue>();
    FREE(state_->getAllocator(), sharedStructures_);
  }
}

ReadError Reader::read(Svalue* pValue) {
  skipSpaces();
  int c = getc();
  switch (c) {
  case '(':
    return readList(pValue);
  case ')':
    return EXTRA_CLOSE_PAREN;
  case ';':
    skipUntilNextLine();
    return read(pValue);
  case '\'':
    return readQuote(pValue);
  case '`':
    return readQuasiQuote(pValue);
  case ',':
    {
      c = getc();
      if (c == '@')
        return readUnquoteSplicing(pValue);
      putback(c);
      return readUnquote(pValue);
    }
  case '"':
    return readString(c, pValue);
  case '#':
    return readSpecial(pValue);
  case EOF:
    return END_OF_FILE;
  default:
    if (!isDelimiter(c)) {
      putback(c);
      return readSymbolOrNumber(pValue);
      return READ_SUCCESS;
    }
    return ILLEGAL_CHAR;
  }
}

ReadError Reader::readSymbolOrNumber(Svalue* pValue) {
  char buffer[256];
  char* p = buffer;
  bool hasSymbolChar = false;
  bool hasDigit = false;
  bool hasDot = false;
  int c;
  while (!isDelimiter(c = getc())) {
    if (isdigit(c)) {
      hasDigit = true;
    } else if (c == '.') {
      hasDot = true;
    } else {
      if (p != buffer || (c != '-' && c != '+'))
        hasSymbolChar = true;
    }
    *p++ = c;
  }
  putback(c);
  *p++ = '\0';
  assert(p - buffer < (int)(sizeof(buffer) / sizeof(*buffer)));

  if (strcmp(buffer, ".") == 0)
    return DOT_AT_BASE;

  if (hasSymbolChar || !hasDigit)
    *pValue = state_->intern(buffer);
  else if (hasDot)
    *pValue = state_->floatValue(static_cast<float>(atof(buffer)));
  else
    *pValue = state_->fixnumValue(atol(buffer));
  return READ_SUCCESS;
}

ReadError Reader::readList(Svalue* pValue) {
  Svalue value = state_->nil();
  Svalue v;
  ReadError err;
  for (;;) {
    err = read(&v);
    if (err != READ_SUCCESS)
      break;
    value = state_->cons(v, value);
  }

  switch (err) {
  case EXTRA_CLOSE_PAREN:
    *pValue = nreverse(state_, value);
    return READ_SUCCESS;
  case END_OF_FILE:
    *pValue = nreverse(state_, value);
    return NO_CLOSE_PAREN;
  case DOT_AT_BASE:
    {
      if (value.eq(state_->nil()))
        return ILLEGAL_CHAR;

      Svalue tail;
      err = read(&tail);
      if (err != READ_SUCCESS)
        return err;

      skipSpaces();
      int c = getc();
      if (c != ')') {
        putback(c);
        return NO_CLOSE_PAREN;
      }

      Svalue lastPair = value;
      *pValue = nreverse(state_, value);
      static_cast<Cell*>(lastPair.toObject())->setCdr(tail);
      return READ_SUCCESS;
    }
    break;
  default:
    return err;
  }
}

ReadError Reader::readQuote(Svalue* pValue) {
  ReadError err = read(pValue);
  if (err == READ_SUCCESS)
    *pValue = list(state_, state_->getConstant(State::QUOTE),*pValue);
  return err;
}

ReadError Reader::readAbbrev(const char* funcname, Svalue* pValue) {
  ReadError err = read(pValue);
  if (err == READ_SUCCESS) {
    *pValue = list(state_,
                   state_->intern(funcname),
                   *pValue);
  }
  return err;
}

ReadError Reader::readSpecial(Svalue* pValue) {
  int c = getc();
  if (!isdigit(c))
    return ILLEGAL_CHAR;

  char buffer[256];
  char* p = buffer;
  do {
    *p++ = c;
    c = getc();
  } while (isdigit(c));
  *p++ = '\0';
  assert(p - buffer < (int)(sizeof(buffer) / sizeof(*buffer)));

  int n = atoi(buffer);
  switch (c) {
  case '=':
    {
      ReadError err = read(pValue);
      if (err != READ_SUCCESS)
        return err;
      storeShared(n, *pValue);
      return READ_SUCCESS;
    }
  case '#':
    if (sharedStructures_ != NULL) {
      const Svalue* p = sharedStructures_->get(n);
      if (p == NULL)
        return ILLEGAL_CHAR;
      *pValue = *p;
      return READ_SUCCESS;
    }
    // Fall
  default:
    return ILLEGAL_CHAR;
  }
}

ReadError Reader::readString(char closeChar, Svalue* pValue) {
  char buffer[256];
  char* p = buffer;
  int c;
  for (;;) {
    c = getc();
    if (c == EOF)
      return NO_CLOSE_STRING;
    if (c == closeChar)
      break;
    switch (c) {
    case '\\':
      c = getc();
      if (c == EOF)
        return NO_CLOSE_STRING;
      if (c != closeChar)
        switch (c) {
        case 'n':  c = '\n'; break;
        case 'r':  c = '\r'; break;
        case 't':  c = '\t'; break;
        default:   break;
        }
      break;
    }
    *p++ = c;
  }
  *p++ = '\0';
  assert(p - buffer < (int)(sizeof(buffer) / sizeof(*buffer)));
  *pValue = state_->stringValue(buffer);
  return READ_SUCCESS;
}

void Reader::storeShared(int id, Svalue value) {
  if (sharedStructures_ == NULL) {
    void* memory = ALLOC(state_->getAllocator(), sizeof(*sharedStructures_));
    sharedStructures_ = new(memory) HashTable<int, Svalue>(&s_hashPolicy,
                                                           state_->getAllocator());
  }
  sharedStructures_->put(id, value);
}

void Reader::skipSpaces() {
  int c;
  while (isSpace(c = getc()))
    ;
  putback(c);
}

void Reader::skipUntilNextLine() {
  int c;
  while (c = getc(), c != '\n' && c != EOF)
    ;
  putback(c);
}

int Reader::getc() {
  return istrm_.get();
}

void Reader::putback(char c) {
  istrm_.putback(c);
}

bool Reader::isSpace(char c) {
  switch (c) {
  case ' ': case '\t': case '\n':
    return true;
  default:
    return false;
  }
}

bool Reader::isDelimiter(char c) {
  switch (c) {
  case ' ': case '\t': case '\n': case '\0': case -1:
  case '(': case ')':
    return true;
  default:
    return false;
  }
}

}  // namespace yalp
