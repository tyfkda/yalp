//=============================================================================
/// basic - basic functions
//=============================================================================

#ifndef _BASIC_HH_
#define _BASIC_HH_

namespace yalp {

class State;
class Value;

void installBasicFunctions(State* state);

Value s_add(State* state);
Value s_sub(State* state);
Value s_mul(State* state);
Value s_div(State* state);
Value s_lessThan(State* state);
Value s_lessEqual(State* state);
Value s_greaterThan(State* state);
Value s_greaterEqual(State* state);

}  // namespace yalp

#endif
