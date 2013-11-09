//=============================================================================
/// Read S-Expression
//=============================================================================

#ifndef _READ_HH_
#define _READ_HH_

#include "macp.hh"

namespace macp {

Svalue readFromString(State* state, const char* str);

}  // namespace macp

#endif
