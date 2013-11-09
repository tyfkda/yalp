//=============================================================================
/// MACP - Macro Processor.
//=============================================================================

#include "macp.hh"

namespace macp {

State* State::create() {
  return new State;
}

State::State() {
}

State::~State() {
}

}  // namespace macp
