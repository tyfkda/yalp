//=============================================================================
/// Read S-Expression
//=============================================================================

#ifndef _READ_HH_
#define _READ_HH_

#include "yalp.hh"

namespace yalp {

enum ReadError {
  SUCCESS,
  END_OF_FILE,
  NO_CLOSE_PAREN,
  NO_CLOSE_STRING,
  ILLEGAL_CHAR,
};


ReadError readFromString(State* state, const char* str, Svalue* pValue);
ReadError readFromFile(State* state, const char* fileName, Svalue* pValue);

}  // namespace yalp

#endif
