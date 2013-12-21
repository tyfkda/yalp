#include "yalp.hh"

using yalp::Fixnum;
using yalp::State;
using yalp::Svalue;

Svalue square(State* state) {
  Fixnum x = state->getArg(0).toFixnum();
  return state->fixnumValue(x * x);
}

int main() {
  State* state = State::create();
  state->runBinaryFromFile("../boot.bin");

  state->defineNative("square", square, 1);
  state->runFromString("(print (square 1111))");

  state->release();
  return 0;
}
