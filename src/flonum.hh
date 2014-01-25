//=============================================================================
/// flonum - flonum functions
//=============================================================================

#ifndef _FLONUM_HH_
#define _FLONUM_HH_

namespace yalp {

class State;

void installFlonumFunctions(State* state);

Value s_addFlonum(State*);
Value s_subFlonum(State*);
Value s_mulFlonum(State*);
Value s_divFlonum(State*);
Value s_lessThanFlonum(State*);
Value s_lessEqualFlonum(State*);
Value s_greaterThanFlonum(State*);
Value s_greaterEqualFlonum(State*);

}  // namespace yalp

#endif
