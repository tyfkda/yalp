#include "yalp.hh"

using yalp::Fixnum;
using yalp::State;
using yalp::Value;
using yalp::bootBinaryData;

Value square(State* state) {
  Fixnum x = state->getArg(0).toFixnum();
  return Value(x * x);
}

int main() {
  State* state = State::create();
  state->runBinaryFromString(bootBinaryData);

  state->defineNative("square", square, 1);
  state->runFromString("(print (square 1111))");

  state->release();
  return 0;
}
