//=============================================================================
/// Read S-Expression
//=============================================================================

#include "read.hh"
#include <assert.h>
#include <fstream>
#include <map>
#include <sstream>

namespace yalp {

// Inner value.
const ReadError CLOSE_PAREN = (ReadError)(ILLEGAL_CHAR + 1);

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
    case '\'':
      return readQuote(pValue);
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

  ReadError readQuote(Svalue* pValue) {
    ReadError err = read(pValue);
    if (err == SUCCESS) {
      *pValue = state_->quote(*pValue);
    }
    return err;
  }

  ReadError readSpecial(Svalue* pValue) {
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
        auto it = sharedStructures_.find(n);
        if (it == sharedStructures_.end())
          return ILLEGAL_CHAR;
        *pValue = it->second;
        return SUCCESS;
      }
    default:
      return ILLEGAL_CHAR;
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
    case '(': case ')': case '\'': case '#':
      return true;
    default:
      return false;
    }
  }

  void storeShared(int id, Svalue value) {
    sharedStructures_[id] = value;
  }

  State* state_;
  std::istream& istrm_;
  std::map<int, Svalue> sharedStructures_;
};

ReadError readFromString(State* state, const char* str, Svalue* pValue) {
  std::istringstream strm(str);
  Reader reader(state, strm);
  return reader.read(pValue);
}

ReadError readFromFile(State* state, const char* fileName, Svalue* pValue) {
  std::ifstream strm(fileName);
  Reader reader(state, strm);
  return reader.read(pValue);
}

}  // namespace yalp
