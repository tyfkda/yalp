//=============================================================================
/// Utility functions
//=============================================================================

#ifndef _YALP_UTIL_HH_
#define _YALP_UTIL_HH_

#include <stdarg.h>

namespace yalp {

class State;
class Stream;
class Svalue;

// Hash function
unsigned int strHash(const char* s);

Svalue list(State* state, Svalue v1);
Svalue list(State* state, Svalue v1, Svalue v2);
Svalue list(State* state, Svalue v1, Svalue v2, Svalue v3);
Svalue nreverse(Svalue v);
int length(Svalue v);

// Print format.
void format(State* state, Stream* out, const char* fmt, ...);
void format(State* state, Stream* out, const char* fmt, va_list ap);

}  // namespace yalp

#endif
