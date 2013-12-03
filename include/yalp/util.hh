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
Svalue nreverse(State* state, Svalue v);
int length(State* state, Svalue v);

}  // namespace yalp

#endif
