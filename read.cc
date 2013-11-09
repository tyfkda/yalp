//=============================================================================
/// Read S-Expression
//=============================================================================

#include "read.hh"
#include <assert.h>
#include <sstream>

namespace macp {

// Inner value.
const ReadError END_OF_FILE = (ReadError)(NO_CLOSE_PAREN + 1);
const ReadError CLOSE_PAREN = (ReadError)(NO_CLOSE_PAREN + 2);

class Reader {
public:
  Reader(State* state, std::istream& istrm)
    : state_(state), istrm_(istrm) {}

  ReadError read(Svalue* pValue) {
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
    case EOF:
      return END_OF_FILE;
    default:
      if (isalnum(c)) {
        unget();
        *pValue = readSymbolOrNumber();
        return SUCCESS;
      }
      return ILLEGAL_CHAR;
    }
  }

private:
  Svalue readSymbolOrNumber() {
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

  ReadError readList(Svalue* pValue) {
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

  void skipSpaces() {
    while (isSpace(getc()))
      ;
    unget();
  }

  void skipUntilNextLine() {
    int c;
    while (c = getc(), c != '\n' && c != EOF)
      ;
    unget();
  }

  static bool isSpace(char c) {
    switch (c) {
    case ' ': case '\t': case '\n':
      return true;
    default:
      return false;
    }
  }

  int getc() {
    return istrm_.get();
  }

  void unget() {
    istrm_.unget();
  }

  static bool isDelimiter(char c) {
    switch (c) {
    case ' ': case '\t': case '\n': case '\0': case -1:
    case '(': case ')': case '#':
      return true;
    default:
      return false;
    }
  }

  State* state_;
  std::istream& istrm_;
};

ReadError readFromString(State* state, const char* str, Svalue* pValue) {
  std::istringstream strm(str);
  Reader reader(state, strm);
  return reader.read(pValue);
}

}  // namespace macp
