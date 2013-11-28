//=============================================================================
/// Read S-Expression
//=============================================================================

#ifndef _READ_HH_
#define _READ_HH_

#include "yalp.hh"
#include <istream>

namespace yalp {

template <class Key, class Value>
class HashTable;

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
  Reader(State* state, std::istream& istrm);
  ~Reader();

  // Reads one s-expression from stream.
  ReadError read(Svalue* pValue);

private:
  ReadError readSymbolOrNumber(Svalue* pValue);
  ReadError readList(Svalue* pValue);
  ReadError readQuote(Svalue* pValue);
  ReadError readQuasiQuote(Svalue* pValue)  { return readAbbrev("quasiquote", pValue); }
  ReadError readUnquote(Svalue* pValue)  { return readAbbrev("unquote", pValue); }
  ReadError readUnquoteSplicing(Svalue* pValue)  { return readAbbrev("unquote-splicing", pValue); }
  ReadError readAbbrev(const char* funcname, Svalue* pValue);
  ReadError readSpecial(Svalue* pValue);
  ReadError readString(char closeChar, Svalue* pValue);
  void storeShared(int id, Svalue value);
  void skipSpaces();
  void skipUntilNextLine();
  int getc();
  void putback(char c);
  static bool isSpace(char c);
  static bool isDelimiter(char c);

  struct HashPolicy;
  static HashPolicy s_hashPolicy;

  State* state_;
  std::istream& istrm_;
  HashTable<int, Svalue>* sharedStructures_;
};

}  // namespace yalp

#endif
