//=============================================================================
/// Binder types
//=============================================================================

#ifndef _BINDER_TYPES_HH_
#define _BINDER_TYPES_HH_

#include "binder.hh"
#include "yalp.hh"
#include "yalp/object.hh"

#include <string>

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
  static Value ret(State*, int i)  { return Value(i); }
};

// float
template<>
struct Type<float> {
  static const char TYPE_NAME[];
  static int check(Value v)  { yalp::Type type = v.getType(); return type == TT_FLONUM || type == TT_FIXNUM; }
  static float get(State* state, Value v)  { return v.toFlonum(state); }
  static Value ret(State* state, float f)  { return state->flonum(f); }
};

// double
template<>
struct Type<double> {
  static const char TYPE_NAME[];
  static int check(Value v)  { yalp::Type type = v.getType(); return type == TT_FLONUM || type == TT_FIXNUM; }
  static double get(State* state, Value v)  { return v.toFlonum(state); }
  static Value ret(State* state, double f)  { return state->flonum(f); }
};

// const char* (string)
template<>
struct Type<const char*> {
  static const char TYPE_NAME[];
  static int check(Value v)  { return v.getType() == TT_STRING; }
  static const char* get(State*, Value v)  { return static_cast<String*>(v.toObject())->c_str(); }
  static Value ret(State* state, const char* str)  { return state->string(str); }
};

// std::string
template<>
struct Type<std::string> {
  static const char TYPE_NAME[];
  static int check(Value v)  { return v.getType() == TT_STRING; }
  static const std::string get(State*, Value v)  { return std::string(static_cast<String*>(v.toObject())->c_str()); }
  static Value ret(State* state, const std::string& str)  { return state->string(str.c_str()); }
};

// std::string
template<>
struct Type<const std::string&> {
  static const char TYPE_NAME[];
  static int check(Value v)  { return v.getType() == TT_STRING; }
  static const std::string get(State*, Value v)  { return std::string(static_cast<String*>(v.toObject())->c_str()); }
  static Value ret(State* state, const std::string& str)  { return state->string(str.c_str()); }
};

// Value
template<>
struct Type<Value> {
  static const char TYPE_NAME[];
  static int check(Value)  { return true; }
  static Value get(State*, Value v)  { return v; }
  static Value ret(State*, Value v)  { return v; }
};

}  // namespace bind
}  // namespace yalp

#endif
