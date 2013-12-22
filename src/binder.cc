//=============================================================================
/// Binder
//=============================================================================

#include "yalp/binder.hh"
#include "yalp/object.hh"

#include <new>

namespace yalp {
namespace bind {

const char Type<int>::TYPE_NAME[] = "Fixnum";
const char Type<double>::TYPE_NAME[] = "Flonum";

class BindedNativeFunc : public NativeFunc {
public:
  BindedNativeFunc(void* funcPtr, int argNum, NativeFuncType bindingFunc)
    : NativeFunc(bindingFunc, argNum, argNum)
    , funcPtr_(funcPtr)  {}

  void* getFuncPtr() const  { return funcPtr_; }

protected:
  void* funcPtr_;
};

Value createBindingFunc(State* state, void* funcPtr,
                        NativeFuncType bindingFunc, int nParam) {
  void* memory = state->objAlloc(sizeof(BindedNativeFunc));
  BindedNativeFunc* nativeFunc = new(memory) BindedNativeFunc(
      static_cast<void*>(funcPtr), nParam, bindingFunc);
  return Value(nativeFunc);
}

void* getBindedFuncPtr(State* state) {
  BindedNativeFunc* fn = static_cast<BindedNativeFunc*>(state->getFunc());
  return fn->getFuncPtr();
}

Value raiseTypeError(State* state, int parameterIndex,
                     const char* requiredTypeName, Value value) {
  state->runtimeError("Type error: parameter %d requires type %s, but %@",
                      parameterIndex + 1, requiredTypeName, &value);
  return Value::NIL;
}

}  // namespace bind
}  // namespace yalp
