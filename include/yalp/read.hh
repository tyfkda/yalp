//=============================================================================
/// Read S-Expression
//=============================================================================

#ifndef _READ_HH_
#define _READ_HH_

#include "yalp.hh"
#include "yalp/error_code.hh"

namespace yalp {

template <class Key, class Value>
class HashTable;
class Stream;

class Reader {
public:
  Reader(State* state, Stream* stream);
  ~Reader();

  // Reads one s-expression from stream.
  ErrorCode read(Svalue* pValue);
  ErrorCode readDelimitedList(int terminator, Svalue* pValue);

private:
  ErrorCode readSymbolOrNumber(Svalue* pValue);
  ErrorCode readList(Svalue* pValue);
  ErrorCode readQuote(Svalue* pValue);
  ErrorCode readQuasiQuote(Svalue* pValue)  { return readAbbrev("quasiquote", pValue); }
  ErrorCode readUnquote(Svalue* pValue)  { return readAbbrev("unquote", pValue); }
  ErrorCode readUnquoteSplicing(Svalue* pValue)  { return readAbbrev("unquote-splicing", pValue); }
  ErrorCode readAbbrev(const char* funcname, Svalue* pValue);
  ErrorCode readString(char closeChar, Svalue* pValue);
  ErrorCode readSpecial(Svalue* pValue);
  ErrorCode readSharedStructure(Svalue* pValue);
  ErrorCode readChar(Svalue* pValue);
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
