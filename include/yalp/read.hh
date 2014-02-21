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

  // Reads s-expressions until terminator is appearred,
  // and returns them in a list.
  ErrorCode readDelimitedList(int terminator, Value* pValue);

  inline int getLineNumber() const  { return lineNo_; }

private:
  inline static bool isSpace(int c);
  inline int getc();
  inline void ungetc(int c);
  ErrorCode readSymbolOrNumber(Value* pValue);
  ErrorCode readString(char closeChar, Value* pValue);
  ErrorCode readSpecial(Value* pValue);
  ErrorCode readSharedStructure(Value* pValue);
  ErrorCode readChar(Value* pValue);
  void storeShared(int id, Value value);
  void skipSpaces();
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
  int lineNo_;
};

}  // namespace yalp

#endif
