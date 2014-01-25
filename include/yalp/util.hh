//=============================================================================
/// Utility functions
//=============================================================================

#ifndef _YALP_UTIL_HH_
#define _YALP_UTIL_HH_

#include "yalp/config.hh"
#include "yalp/error_code.hh"
#include <stdarg.h>

namespace yalp {

class Reader;
class State;
class Stream;
class Value;

Value car(Value s);
Value cdr(Value s);

Value list(State* state, Value v1);
Value list(State* state, Value v1, Value v2);
Value list(State* state, Value v1, Value v2, Value v3);
Value nreverse(Value v);
int length(Value v);
Value listToVector(State* state, Value ls);

// Print format.
void format(State* state, Stream* out, const char* fmt, ...);
void format(State* state, Stream* out, const char* fmt, va_list ap);
void format(State* state, Stream* out, const char* fmt, const Value* values);

void raiseReadError(State* state, ErrorCode err, Reader* reader);

}  // namespace yalp

#endif
