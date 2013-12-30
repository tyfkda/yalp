//=============================================================================
/// Binder
//=============================================================================

#ifndef _BINDER_HH_
#define _BINDER_HH_

#include "yalp.hh"
#include "binder_types.hh"

namespace yalp {
namespace bind {

Value createBindingFunc(State* state, void* funcPtr,
                        NativeFuncType bindingFunc, int nParam);
void* getBindedFuncPtr(State* state);
Value raiseTypeError(State* state, int parameterIndex,
                     const char* requiredTypeName, Value value);

#include "binder.inc"

class Binder {
public:
  Binder(State* state) : state_(state)  {}

  template <class Func>
  void bind(const char* name, Func funcPtr) {
    int arena = state_->saveArena();
    Value func = createBindingFunc(state_, (void*)funcPtr,
                                   Selector<Func>::call,
                                   Selector<Func>::NPARAM);
    state_->defineGlobal(name, func);
    state_->restoreArena(arena);
  }

private:
  State* state_;
};

}  // namespace bind
}  // namespace yalp

#endif
