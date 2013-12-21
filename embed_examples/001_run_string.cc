#include "yalp.hh"

using yalp::State;

int main() {
  State* state = State::create();
  state->runBinaryFromFile("../boot.bin");

  state->runFromString("(print \"Hello, Yalp!\")");

  state->release();
  return 0;
}
