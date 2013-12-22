//=============================================================================
/// Binder types
//=============================================================================

#ifndef _BINDER_TYPES_HH_
#define _BINDER_TYPES_HH_

#include "binder.hh"
#include "yalp.hh"

namespace yalp {
namespace bind {

template <class T>
struct Type {
  //static int check(Value v) = 0;
  //static int get(State*, Value v) = 0;
  //static Value ret(State*, int f) = 0;
};

// int
template<>
struct Type<int> {
  static const char TYPE_NAME[];
  static int check(Value v)  { return v.getType() == TT_FIXNUM; }
  static int get(State*, Value v)  { return v.toFixnum(); }
  static Value ret(State*, int f)  { return Value(f); }
};

// double
template<>
struct Type<double> {
  static const char TYPE_NAME[];
  static int check(Value v)  { yalp::Type type = v.getType(); return type == TT_FLONUM || type == TT_FIXNUM; }
  static double get(State* state, Value v)  { return v.toFlonum(state); }
  static Value ret(State* state, double f)  { return state->flonum(f); }
};

}  // namespace bind
}  // namespace yalp

#endif
