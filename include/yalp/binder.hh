//=============================================================================
/// Binder
//=============================================================================

#ifndef _BINDER_HH_
#define _BINDER_HH_

#include "yalp.hh"
#include "binder_types.hh"

namespace yalpbind {

yalp::Value createBindingFunc(yalp::State* state, void* funcPtr,
                              yalp::NativeFuncType bindingFunc, int nParam);
void* getBindedFuncPtr(yalp::State* state);
yalp::Value raiseTypeError(yalp::State* state, int parameterIndex,
                           const char* requiredTypeName, yalp::Value value);

#include "binder.inc"

class YalpBind {
public:
  YalpBind(yalp::State* state) : state_(state)  {}

  template <class Func>
  void bind(const char* name, Func funcPtr) {
    yalp::Value func = createBindingFunc(state_, (void*)funcPtr,
                                         Binder<Func>::call,
                                         Binder<Func>::NPARAM);
    state_->defineGlobal(name, func);
  }

private:
  yalp::State* state_;
};

}  // namespace yalpbind

#endif
