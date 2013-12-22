//=============================================================================
/// Binder types
//=============================================================================

#ifndef _BINDER_TYPES_HH_
#define _BINDER_TYPES_HH_

#include "binder.hh"
#include "yalp.hh"

namespace yalpbind {

template <class T>
struct Type {
  //static int check(yalp::Value v) = 0;
  //static int get(yalp::State*, yalp::Value v) = 0;
  //static yalp::Value ret(yalp::State*, int f) = 0;
};

// int
template<>
struct Type<int> {
  static const char TYPE_NAME[];
  static int check(yalp::Value v)  { yalp::Type type = v.getType(); return type == yalp::TT_FIXNUM; }
  static int get(yalp::State*, yalp::Value v)  { return v.toFixnum(); }
  static yalp::Value ret(yalp::State*, int f)  { return yalp::Value(f); }
};

// double
template<>
struct Type<double> {
  static const char TYPE_NAME[];
  static int check(yalp::Value v)  { yalp::Type type = v.getType(); return type == yalp::TT_FLONUM || type == yalp::TT_FIXNUM; }
  static double get(yalp::State* state, yalp::Value v)  { return v.toFlonum(state); }
  static yalp::Value ret(yalp::State* state, double f)  { return state->flonum(f); }
};

}  // namespace yalpbind

#endif
