//=============================================================================
/// Read S-Expression
//=============================================================================

#include "read.hh"
#include <assert.h>

namespace yalp {

// Inner value.
const ReadError CLOSE_PAREN = (ReadError)(ILLEGAL_CHAR + 1);

Reader::Reader(State* state, std::istream& istrm)
  : state_(state), istrm_(istrm)
  , sharedStructures_(new std::map<int, Svalue>()) {
}

Reader::~Reader() {
  delete sharedStructures_;
}

ReadError Reader::read(Svalue* pValue) {
  skipSpaces();
  int c = getc();
  switch (c) {
  case '(':
    return readList(pValue);
  case ')':
    return CLOSE_PAREN;
  case ';':
    skipUntilNextLine();
    return read(pValue);
  case '\'':
    return readQuote(pValue);
  case '"':
    return readString(c, pValue);
  case '#':
    return readSpecial(pValue);
  case EOF:
    return END_OF_FILE;
  default:
    if (!isDelimiter(c)) {
      unget();
      *pValue = readSymbolOrNumber();
      return SUCCESS;
    }
    return ILLEGAL_CHAR;
  }
}

Svalue Reader::readSymbolOrNumber() {
  char buffer[256];
  char* p = buffer;
  bool hasSymbolChar = false;
  int c;
  while (!isDelimiter(c = getc())) {
    if (!isdigit(c))
      hasSymbolChar = true;
    *p++ = c;
  }
  unget();
  *p++ = '\0';
  assert(p - buffer < (int)(sizeof(buffer) / sizeof(*buffer)));
  
  if (hasSymbolChar)
    return state_->intern(buffer);
  else
    return state_->fixnumValue(atol(buffer));
}

ReadError Reader::readList(Svalue* pValue) {
  Svalue value = state_->nil();
  Svalue v;
  ReadError err;
  for (;;) {
    err = read(&v);
    if (err != SUCCESS)
      break;
    value = state_->cons(v, value);
  }
  
  if (err == CLOSE_PAREN) {
    *pValue = nreverse(state_, value);
    return SUCCESS;
  } else if (err == END_OF_FILE) {
    *pValue = nreverse(state_, value);
    return NO_CLOSE_PAREN;
  } else {
    return err;
  }
}

ReadError Reader::readQuote(Svalue* pValue) {
  ReadError err = read(pValue);
  if (err == SUCCESS) {
    *pValue = state_->quote(*pValue);
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
      if (err != SUCCESS)
        return err;
      storeShared(n, *pValue);
      return SUCCESS;
    }
  case '#':
    {
      auto it = sharedStructures_->find(n);
      if (it == sharedStructures_->end())
        return ILLEGAL_CHAR;
      *pValue = it->second;
      return SUCCESS;
    }
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
    *p++ = c;
  }
  *p++ = '\0';
  assert(p - buffer < (int)(sizeof(buffer) / sizeof(*buffer)));
  *pValue = state_->stringValue(buffer);
  return SUCCESS;
}

void Reader::storeShared(int id, Svalue value) {
  (*sharedStructures_)[id] = value;
}

void Reader::skipSpaces() {
  while (isSpace(getc()))
    ;
  unget();
}

void Reader::skipUntilNextLine() {
  int c;
  while (c = getc(), c != '\n' && c != EOF)
    ;
  unget();
}

int Reader::getc() {
  return istrm_.get();
}

void Reader::unget() {
  istrm_.unget();
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
  case '(': case ')': case '\'': case '#':
    return true;
  default:
    return false;
  }
}

}  // namespace yalp
