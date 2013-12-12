//=============================================================================
/// Read S-Expression
//=============================================================================

#ifndef _READ_HH_
#define _READ_HH_

#include "yalp.hh"

namespace yalp {

template <class Key, class Value>
class HashTable;
class Stream;

enum ReadError {
  READ_SUCCESS,
  END_OF_FILE,
  NO_CLOSE_PAREN,
  EXTRA_CLOSE_PAREN,
  NO_CLOSE_STRING,
  DOT_AT_BASE,  // `.' is appeared at base text.
  ILLEGAL_CHAR,
};

class Reader {
public:
  Reader(State* state, Stream* stream);
  ~Reader();

  // Reads one s-expression from stream.
  ReadError read(Svalue* pValue);
  ReadError readDelimitedList(int terminator, Svalue* pValue);

private:
  ReadError readSymbolOrNumber(Svalue* pValue);
  ReadError readList(Svalue* pValue);
  ReadError readQuote(Svalue* pValue);
  ReadError readQuasiQuote(Svalue* pValue)  { return readAbbrev("quasiquote", pValue); }
  ReadError readUnquote(Svalue* pValue)  { return readAbbrev("unquote", pValue); }
  ReadError readUnquoteSplicing(Svalue* pValue)  { return readAbbrev("unquote-splicing", pValue); }
  ReadError readAbbrev(const char* funcname, Svalue* pValue);
  ReadError readString(char closeChar, Svalue* pValue);
  ReadError readSpecial(Svalue* pValue);
  ReadError readSharedStructure(Svalue* pValue);
  ReadError readChar(Svalue* pValue);
  void storeShared(int id, Svalue value);
  void skipSpaces();
  void skipUntilNextLine();
  int getc();
  void putback(char c);
  static bool isSpace(int c);
  static bool isDelimiter(int c);
  static inline int isNotDelimiter(int c)  { return !isDelimiter(c); }

  int readToBufferWhile(char** pBuffer, int* pSize, int (*cond)(int));
  inline int putBuffer(char** pBuffer, int* pSize, int p, int c);

  struct IntHashPolicy;
  static IntHashPolicy s_hashPolicy;

  State* state_;
  Stream* stream_;
  HashTable<int, Svalue>* sharedStructures_;
  char* buffer_;
  int size_;
};

}  // namespace yalp

#endif
