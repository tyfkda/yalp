//=============================================================================
/// Vm - Macro Processor VM.
//=============================================================================

#include "vm.hh"
#include "macp.hh"

namespace macp {

Vm* Vm::create(State* state) {
  return new Vm(state);
}

Vm::~Vm() {}

Vm::Vm(State* state)
  : state_(state)
  , stack_(NULL), stackSize_(0) {
}

Svalue Vm::run(Svalue code) {
  Svalue nil = state_->nil();
  return run(nil, code, 0, nil, 0);
}

Svalue Vm::run(Svalue a, Svalue code, int f, Svalue c, int s) {
  return code;
}

}  // namespace macp
