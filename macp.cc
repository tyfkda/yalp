//=============================================================================
/// MACP - Macro Processor.
//=============================================================================

#include "macp.hh"
#include <stdlib.h>

namespace macp {

State* State::create() {
  return new State;
}

State::State() {
}

State::~State() {
}

Svalue State::readString(const char* str) {
  return atoi(str);
}

}  // namespace macp
