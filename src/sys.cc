//=============================================================================
/// sys - system functions
//=============================================================================

#include "build_env.hh"
#include "sys.hh"
#include "yalp.hh"
#include <sys/time.h>
/*
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"
#include "hash_table.hh"
#include "vm.hh"
*/

namespace yalp {

//=============================================================================
// Native functions

// Returns second and micro-second
static Value s_gettimeofday(State* state) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return state->multiValues(Value(tv.tv_sec), Value(tv.tv_usec));
}

void installSystemFunctions(State* state) {
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  struct {
    const char* name;
    NativeFuncType func;
    int minArgNum, maxArgNum;
  } static const FuncTable[] = {
    { "gettimeofday", s_gettimeofday, 0 },
  };

  for (auto it : FuncTable) {
    int maxArgNum = it.maxArgNum == 0 ? it.minArgNum : it.maxArgNum;
    state->defineNative(it.name, it.func, it.minArgNum, maxArgNum);
  }
}

}  // namespace yalp
