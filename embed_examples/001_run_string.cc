#include "yalp.hh"

using yalp::State;
using yalp::bootBinaryData;

int main() {
  State* state = State::create();
  state->runBinaryFromString(bootBinaryData);

  state->runFromString("(print \"Hello, Yalp!\")");

  state->release();
  return 0;
}
