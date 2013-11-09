//=============================================================================
/// Read S-Expression
//=============================================================================

#ifndef _READ_HH_
#define _READ_HH_

#include "macp.hh"

namespace macp {

enum ReadError {
  SUCCESS,
  NO_CLOSE_PAREN,
  ILLEGAL_CHAR,
};


ReadError readFromString(State* state, const char* str, Svalue* pValue);

}  // namespace macp

#endif
