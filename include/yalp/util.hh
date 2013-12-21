//=============================================================================
/// Utility functions
//=============================================================================

#ifndef _YALP_UTIL_HH_
#define _YALP_UTIL_HH_

#include <stdarg.h>

namespace yalp {

class State;
class Stream;
class Value;

// Hash function
unsigned int strHash(const char* s);

Value list(State* state, Value v1);
Value list(State* state, Value v1, Value v2);
Value list(State* state, Value v1, Value v2, Value v3);
Value nreverse(Value v);
int length(Value v);

// Print format.
void format(State* state, Stream* out, const char* fmt, ...);
void format(State* state, Stream* out, const char* fmt, va_list ap);

}  // namespace yalp

#endif
