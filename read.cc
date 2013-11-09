//=============================================================================
/// Read S-Expression
//=============================================================================

#include "read.hh"
#include <assert.h>
#include <sstream>

namespace macp {

class Reader {
public:
  enum ResultType {
    SUCCESS,
    CLOSE_PAREN,
    DOT,
    END_OF_FILE,
  };

  Reader(State* state, std::istream& istrm)
    : state_(state), istrm_(istrm) {}

  ResultType read(Svalue* pValue) {
    skipSpaces();
    int c = getc();
    switch (c) {
    case '(':
      return readList(pValue);
    case ')':
      return CLOSE_PAREN;
    case '.':
      return DOT;
    case EOF:
      return END_OF_FILE;
    }

    if (isdigit(c)) {
      unget();
      *pValue = readNumber();
      return SUCCESS;
    }
    return END_OF_FILE;
  }

private:
  Svalue readNumber() {
    char buffer[256];
    char* p = buffer;
    int c;
    while (!isDelimiter(c = getc())) {
      *p++ = c;
    }
    unget();
    *p++ = '\0';
    assert(p - buffer < (int)(sizeof(buffer) / sizeof(*buffer)));

    Sfixnum x = atol(buffer);
    return state_->fixnumValue(x);
  }

  ResultType readList(Svalue* pValue) {
    Svalue nil = state_->nil();
    Svalue value = state_->nil();
    ResultType err;
    for (;;) {
      Svalue v = nil;
      err = read(&v);
      if (err != SUCCESS)
        break;
      value = state_->cons(v, value);
    }

    switch (err) {
    case CLOSE_PAREN:
      *pValue = nreverse(state_, value);
      return SUCCESS;
    default:
      return err;
    }
  }

  void skipSpaces() {
    while (isSpace(getc()))
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

Svalue readFromString(State* state, const char* str) {
  std::istringstream strm(str);
  Reader reader(state, strm);
  Svalue value = state->nil();
  reader.read(&value);
  return value;
}

}  // namespace macp
