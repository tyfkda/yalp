//=============================================================================
/// Read S-Expression
//=============================================================================

#include "build_env.hh"
#include "yalp/read.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"
#include "hash_table.hh"

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
  if ('A' <= c && c <= 'Z')
    return c - ('A' - 10);
  if ('a' <= c && c <= 'z')
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

Reader::Reader(State* state, Stream* stream)
  : state_(state), stream_(stream)
  , sharedStructures_(NULL)
  , buffer_(NULL), size_(0), lineNo_(0) {
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
  lineNo_ = stream_->getLineNumber();

  int c = getc();
  // "#dd=" and "#dd#" is shared structure (hard wired).
  if (c == '#') {
    int c2 = getc();
    ungetc(c2);
    if (isdigit(c2))
      return readSharedStructure(pValue);
  }

  Value fn = state_->getMacroCharacter(c);
  if (fn.isTrue()) {
    if (fn.getType() != TT_HASH_TABLE) {  // Single macro character.
      SStream ss(stream_);
      Value args[] = { Value(&ss), state_->character(c) };
      if (!state_->funcall(fn, sizeof(args) / sizeof(*args), args, pValue))
        return ILLEGAL_CHAR;
      if (state_->getResultNum() == 0)
        return read(pValue);
      return SUCCESS;
    }
    // Dispatch macro character.
    int c2 = getc();
    fn = state_->getDispatchMacroCharacter(c, c2);
    if (fn.isTrue()) {
      SStream ss(stream_);
      Value args[] = { Value(&ss), state_->character(c), state_->character(c2) };
      if (state_->funcall(fn, sizeof(args) / sizeof(*args), args, pValue)) {
        if (state_->getResultNum() == 0)
          return read(pValue);
        return SUCCESS;
      }
    }

    if (c == '#') {  // Fall back for temp.
      ungetc(c2);
      return readSpecial(pValue);
    }
    return ILLEGAL_CHAR;
  }

  switch (c) {
  case '(':
    return readDelimitedList(')', pValue);
  case ')': case ']': case '}':
    ungetc(c);
    return EXTRA_CLOSE_PAREN;
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
#ifdef DISABLE_FLONUM
    return ILLEGAL_CHAR;
#else
    *pValue = state_->flonum(static_cast<Flonum>(atof(buffer)));
#endif
  else
    *pValue = Value(atol(buffer));
  return SUCCESS;
}

ErrorCode Reader::readDelimitedList(int terminator, Value* pValue) {
  int arena = state_->saveArena();
  int lineNo = stream_->getLineNumber();
  Value value = Value::NIL;
  Value v;
  ErrorCode err;
  for (;;) {
    skipSpaces();
    err = read(&v);
    if (err != SUCCESS)
      break;
    // TODO: Sometimes value is broken here after reader macro executed.
    value = state_->cons(v, value);
    state_->restoreArenaWith(arena, value);
  }

  switch (err) {
  case END_OF_FILE:
    *pValue = reverseBang(value);
    state_->restoreArenaWith(arena, *pValue);
    lineNo_ = lineNo;
    return NO_CLOSE_PAREN;
  case EXTRA_CLOSE_PAREN: case ILLEGAL_CHAR:
    {
      int c = getc();
      if (c != terminator) {
        ungetc(c);
        return err;
      }
      *pValue = reverseBang(value);
      state_->restoreArenaWith(arena, *pValue);
      return SUCCESS;
    }
  case DOT_AT_BASE:
    {
      if (value.eq(Value::NIL))
        return ILLEGAL_CHAR;

      Value tail;
      err = read(&tail);
      if (err != SUCCESS)
        return err;

      skipSpaces();
      lineNo = stream_->getLineNumber();
      int c = getc();
      if (c != terminator) {
        ungetc(c);
        lineNo_ = lineNo;
        return NO_CLOSE_PAREN;
      }

      Value lastPair = value;
      *pValue = reverseBang(value);
      static_cast<Cell*>(lastPair.toObject())->setCdr(tail);
      state_->restoreArenaWith(arena, *pValue);
      return SUCCESS;
    }
    break;
  default:
    return err;
  }
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
            if (c1 < 0 || c1 >= 16 || c2 < 0 || c2 >= 16)
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
  switch (c) {
  case '\\':
    return readChar(pValue);
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
      // Store temporary object before reading labeled object
      // for self referential.

      // Assumes that labeled ss is a pair.
      Value tmp = state_->cons(Value::NIL, Value::NIL);  // Temporary object.
      storeShared(n, tmp);

      Value object;
      ErrorCode err = read(&object);
      if (err != SUCCESS)
        return err;

      // Copy pair.
      state_->checkType(object, TT_CELL);
      Cell* p = static_cast<Cell*>(tmp.toObject());
      p->setCar(car(object));
      p->setCdr(cdr(object));
      *pValue = tmp;
      return SUCCESS;
    }
  case '#':
    if (sharedStructures_ != NULL) {
      const Value* p = sharedStructures_->get(n);
      if (p != NULL) {
        *pValue = *p;
        return SUCCESS;
      }
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
  readToBufferWhile(&buffer, &size, isNotDelimiter);

  // 1 character?
  {
    unsigned char* q = reinterpret_cast<unsigned char*>(buffer);
    int c = utf8ToUnicode(&q);
    if (c >= 0 && *q == '\0') {
      *pValue = state_->character(c);
      return SUCCESS;
    }
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
  // "\xhhhh" notation.
  if (buffer[0] == 'x') {
    Fixnum x = 0;
    for (char* p = buffer; *++p != '\0';) {
      int h = hexChar(*p);
      if (h < 0)
        return ILLEGAL_CHAR;
      x = (x << 4) | h;
    }
    *pValue = state_->character(x);
    return SUCCESS;
  }
  return ILLEGAL_CHAR;
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

bool Reader::isDelimiter(int c) {
  switch (c) {
  case ' ': case '\t': case '\n': case '\0': case -1:
  case '(': case ')': case '[': case ']': case '{': case '}':
  case ';': case ',':
    return true;
  default:
    return false;
  }
}

}  // namespace yalp
