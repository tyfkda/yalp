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
  ErrorCode read(Value* pValue);
  ErrorCode readDelimitedList(int terminator, Value* pValue);

private:
  inline static bool isSpace(int c);
  inline int getc();
  inline void putback(char c);
  inline ErrorCode readQuote(Value* pValue);
  inline ErrorCode readQuasiQuote(Value* pValue);
  inline ErrorCode readUnquote(Value* pValue);
  inline ErrorCode readUnquoteSplicing(Value* pValue);
  ErrorCode readSymbolOrNumber(Value* pValue);
  ErrorCode readList(Value* pValue);
  ErrorCode readAbbrev(Value symbol, Value* pValue);
  ErrorCode readString(char closeChar, Value* pValue);
  ErrorCode readSpecial(Value* pValue);
  ErrorCode readSharedStructure(Value* pValue);
  ErrorCode readChar(Value* pValue);
  void storeShared(int id, Value value);
  void skipSpaces();
  void skipUntilNextLine();
  static bool isDelimiter(int c);
  static inline int isNotDelimiter(int c)  { return !isDelimiter(c); }

  int readToBufferWhile(char** pBuffer, int* pSize, int (*cond)(int));
  inline int putBuffer(char** pBuffer, int* pSize, int p, int c);

  struct IntHashPolicy;
  static IntHashPolicy s_hashPolicy;

  State* state_;
  Stream* stream_;
  HashTable<int, Value>* sharedStructures_;
  char* buffer_;
  int size_;
};

}  // namespace yalp

#endif
