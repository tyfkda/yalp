//=============================================================================
/// Utility functions
//=============================================================================

#ifndef _YALP_UTIL_HH_
#define _YALP_UTIL_HH_

namespace yalp {

class State;
class Svalue;

Svalue list(State* state, Svalue v1);
Svalue list(State* state, Svalue v1, Svalue v2);
Svalue list(State* state, Svalue v1, Svalue v2, Svalue v3);
Svalue nreverse(Svalue v);
int length(Svalue v);

}  // namespace yalp

#endif
