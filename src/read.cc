//=============================================================================
/// Read S-Expression
//=============================================================================

#include "yalp/read.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"
#include "hash_table.hh"

#include <alloca.h>
#include <ctype.h>  // for isdigit
#include <new>
#include <stdlib.h>  // for atof
#include <string.h>  // for memcpy

namespace yalp {

const int DEFAULT_SIZE = 24;

#define BUFFER(buffer, size)                            \
  char* buffer;                                         \
  int size;                                             \
  do {                                                  \
    if (buffer_ != NULL) {                              \
      buffer = buffer_;                                 \
      size = size_;                                     \
    } else {                                            \
      size = DEFAULT_SIZE;                              \
      buffer = static_cast<char*>(alloca(size));        \
    }                                                   \
  } while (0)

static int hexChar(int c) {
  if (isdigit(c))
    return c - '0';
  if ('A' <= c && c <= 'F')
    return c - ('A' - 10);
  if ('a' <= c && c <= 'f')
    return c - ('a' - 10);
  return -1;
}

struct Reader::IntHashPolicy : public HashPolicy<int> {
  virtual unsigned int hash(int a) override  { return a; }
  virtual bool equal(int a, int b) override  { return a == b; }
};

Reader::IntHashPolicy Reader::s_hashPolicy;

bool Reader::isSpace(int c) {
  switch (c) {
  case ' ': case '\t': case '\n': return true;
  default: return false;
  }
}

int Reader::getc()  { return stream_->get(); }
void Reader::ungetc(int c)  { stream_->ungetc(c); }

ErrorCode Reader::readQuote(Value* pValue)  { return readAbbrev(state_->getConstant(State::QUOTE), pValue); }
ErrorCode Reader::readQuasiQuote(Value* pValue)  { return readAbbrev(state_->getConstant(State::QUASIQUOTE), pValue); }
ErrorCode Reader::readUnquote(Value* pValue)  { return readAbbrev(state_->getConstant(State::UNQUOTE), pValue); }
ErrorCode Reader::readUnquoteSplicing(Value* pValue)  { return readAbbrev(state_->getConstant(State::UNQUOTE_SPLICING), pValue); }

Reader::Reader(State* state, Stream* stream)
  : state_(state), stream_(stream)
  , sharedStructures_(NULL)
  , buffer_(NULL), size_(0) {
}

Reader::~Reader() {
  if (buffer_ != NULL)
    state_->free(buffer_);
  if (sharedStructures_ != NULL) {
    sharedStructures_->~HashTable<int, Value>();
    state_->free(sharedStructures_);
  }
}

ErrorCode Reader::read(Value* pValue) {
  skipSpaces();
  int c = getc();
  Value fn = state_->getMacroCharacter(c);
  if (fn.isTrue()) {
    SStream ss(stream_);
    Value args[] = { Value(&ss), state_->character(c) };
    if (state_->funcall(fn, sizeof(args) / sizeof(*args), args, pValue))
      return SUCCESS;
    return ILLEGAL_CHAR;
  }

  switch (c) {
  case '(':
    return readList(pValue);
  case ')': case ']': case '}':
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
      ungetc(c);
      return readUnquote(pValue);
    }
  case '"':
    return readString(c, pValue);
  case '#':
    return readSpecial(pValue);
  case EOF:
    return END_OF_FILE;
  default:
    ungetc(c);
    if (!isDelimiter(c)) {
      return readSymbolOrNumber(pValue);
    }
    return ILLEGAL_CHAR;
  }
}

int Reader::putBuffer(char** pBuffer, int* pSize, int p, int c) {
  if (p >= *pSize) {
    int newSize;
    for (newSize = *pSize; (newSize <<= 1) <= p; );
    char* newBuffer;
    if (buffer_ == NULL) {  // Allocates memory from heap at first time.
      newBuffer = static_cast<char*>(state_->alloc(newSize));
      memcpy(newBuffer, *pBuffer, *pSize);
    } else {  // Already allocated memory from heap, and expand it.
      newBuffer = static_cast<char*>(state_->realloc(*pBuffer, newSize));
    }
    *pBuffer = buffer_ = newBuffer;
    *pSize = size_ = newSize;
  }
  (*pBuffer)[p] = c;
  return p + 1;
}

int Reader::readToBufferWhile(char** pBuffer, int* pSize, int (*cond)(int)) {
  int p = 0;
  int c;
  while (cond(c = getc()))
    p = putBuffer(pBuffer, pSize, p, c);
  ungetc(c);
  putBuffer(pBuffer, pSize, p, '\0');
  return p;
}

ErrorCode Reader::readSymbolOrNumber(Value* pValue) {
  BUFFER(buffer, size);
  readToBufferWhile(&buffer, &size, isNotDelimiter);

  if (strcmp(buffer, ".") == 0)
    return DOT_AT_BASE;

  bool hasSymbolChar = false;
  bool hasDigit = false;
  bool hasDot = false;
  for (char* p = buffer; *p != '\0'; ++p) {
    unsigned char c = *p;
    if (isdigit(c)) {
      hasDigit = true;
    } else if (c == '.') {
      hasDot = true;
    } else {
      if (p != buffer || (c != '-' && c != '+'))
        hasSymbolChar = true;
    }
  }

  if (hasSymbolChar || !hasDigit)
    *pValue = state_->intern(buffer);
  else if (hasDot)
    *pValue = state_->flonum(static_cast<Flonum>(atof(buffer)));
  else
    *pValue = Value(atol(buffer));
  return SUCCESS;
}

ErrorCode Reader::readList(Value* pValue) {
  return readDelimitedList(')', pValue);
}

ErrorCode Reader::readDelimitedList(int terminator, Value* pValue) {
  Value value = Value::NIL;
  Value v;
  ErrorCode err;
  for (;;) {
    skipSpaces();
    int c = getc();
    if (c == terminator) {
      *pValue = nreverse(value);
      return SUCCESS;
    }

    ungetc(c);
    err = read(&v);
    if (err != SUCCESS)
      break;
    // TODO: Sometimes value is broken here after reader macro executed.
    value = state_->cons(v, value);
  }

  switch (err) {
  case END_OF_FILE:
    *pValue = nreverse(value);
    return NO_CLOSE_PAREN;
  case DOT_AT_BASE:
    {
      if (value.eq(Value::NIL))
        return ILLEGAL_CHAR;

      Value tail;
      err = read(&tail);
      if (err != SUCCESS)
        return err;

      skipSpaces();
      int c = getc();
      if (c != terminator) {
        ungetc(c);
        return NO_CLOSE_PAREN;
      }

      Value lastPair = value;
      *pValue = nreverse(value);
      static_cast<Cell*>(lastPair.toObject())->setCdr(tail);
      return SUCCESS;
    }
    break;
  default:
    return err;
  }
}

ErrorCode Reader::readAbbrev(Value symbol, Value* pValue) {
  ErrorCode err = read(pValue);
  if (err == SUCCESS)
    *pValue = list(state_, symbol, *pValue);
  return err;
}

ErrorCode Reader::readString(char closeChar, Value* pValue) {
  BUFFER(buffer, size);

  int p = 0;
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
        return ILLEGAL_CHAR;
      if (c != closeChar) {
        switch (c) {
        case '0':  c = '\0'; break;
        case 'n':  c = '\n'; break;
        case 'r':  c = '\r'; break;
        case 't':  c = '\t'; break;
        case 'x':
          {
            int c1 = hexChar(getc());
            int c2 = hexChar(getc());
            if (c1 < 0 || c2 < 0)
              return ILLEGAL_CHAR;
            c = (c1 << 4) | c2;
          }
          break;
        default:   break;
        }
      }
      break;
    }
    p = putBuffer(&buffer, &size, p, c);
  }
  putBuffer(&buffer, &size, p, '\0');
  *pValue = state_->string(buffer, p);
  return SUCCESS;
}

ErrorCode Reader::readSpecial(Value* pValue) {
  int c = getc();
  if (isdigit(c)) {
    ungetc(c);
    return readSharedStructure(pValue);
  }
  switch (c) {
  case '\\':
    return readChar(pValue);
  case '.':
    return readTimeEval(pValue);
  default:
    ungetc(c);
    return ILLEGAL_CHAR;
  }
}

ErrorCode Reader::readSharedStructure(Value* pValue) {
  BUFFER(buffer, size);
  readToBufferWhile(&buffer, &size, isdigit);

  int n = atoi(buffer);
  int c = getc();
  switch (c) {
  case '=':
    {
      ErrorCode err = read(pValue);
      if (err != SUCCESS)
        return err;
      storeShared(n, *pValue);
      return SUCCESS;
    }
  case '#':
    if (sharedStructures_ != NULL) {
      const Value* p = sharedStructures_->get(n);
      if (p == NULL)
        return ILLEGAL_CHAR;
      *pValue = *p;
      return SUCCESS;
    }
    // Fall
  default:
    ungetc(c);
    return ILLEGAL_CHAR;
  }
}

ErrorCode Reader::readChar(Value* pValue) {
  int c = getc();
  switch (c) {
  case ' ': case '\n': case '\t': case -1:
    return ILLEGAL_CHAR;
  default:
    if (isDelimiter(c)) {
      *pValue = state_->character(c);
      return SUCCESS;
    }
    ungetc(c);
    break;
  }

  BUFFER(buffer, size);
  int p = readToBufferWhile(&buffer, &size, isNotDelimiter);

  if (p == 1) {
    *pValue = state_->character(reinterpret_cast<unsigned char*>(buffer)[0]);
    return SUCCESS;
  }

  struct {
    const char* name;
    int code;
  } static const Table[] = {
    { "space", ' ' },
    { "nl", '\n' },
    { "newline", '\n' },
    { "tab", '\t' },
    { "escape", 0x1b },
  };
  for (unsigned int i = 0; i < sizeof(Table) / sizeof(*Table); ++i) {
    if (strcmp(buffer, Table[i].name) == 0) {
      *pValue = state_->character(Table[i].code);
      return SUCCESS;
    }
  }
  return ILLEGAL_CHAR;
}

ErrorCode Reader::readTimeEval(Value* pValue) {
  Value exp;
  ErrorCode err = read(&exp);
  if (err != SUCCESS)
    return err;

  Value code;
  if (!state_->compile(exp, &code))
    return COMPILE_ERROR;
  if (!state_->runBinary(code, pValue))
    return RUNTIME_ERROR;
  return SUCCESS;
}

void Reader::storeShared(int id, Value value) {
  if (sharedStructures_ == NULL) {
    void* memory = state_->alloc(sizeof(*sharedStructures_));
    sharedStructures_ = new(memory) HashTable<int, Value>(&s_hashPolicy,
                                                          state_->getAllocator());
  }
  sharedStructures_->put(id, value);
}

void Reader::skipSpaces() {
  int c;
  while (isSpace(c = getc()))
    ;
  ungetc(c);
}

void Reader::skipUntilNextLine() {
  int c;
  while (c = getc(), c != '\n' && c != EOF)
    ;
  ungetc(c);
}

bool Reader::isDelimiter(int c) {
  switch (c) {
  case ' ': case '\t': case '\n': case '\0': case -1:
  case '(': case ')': case '[': case ']': case '{': case '}':
  case ';':
    return true;
  default:
    return false;
  }
}

}  // namespace yalp
