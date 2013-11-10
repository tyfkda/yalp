//=============================================================================
/// Read S-Expression
//=============================================================================

#ifndef _READ_HH_
#define _READ_HH_

#include "yalp.hh"

namespace yalp {

enum ReadError {
  SUCCESS,
  NO_CLOSE_PAREN,
  ILLEGAL_CHAR,
};


ReadError readFromString(State* state, const char* str, Svalue* pValue);
ReadError readFromFile(State* state, const char* fileName, Svalue* pValue);

}  // namespace yalp

#endif
