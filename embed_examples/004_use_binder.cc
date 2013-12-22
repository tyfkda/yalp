#include "yalp.hh"
#include "yalp/binder.hh"

#include <iostream>

using yalp::Fixnum;
using yalp::State;
using yalp::Value;

int square(int x) {
  return x * x;
}

int main() {
  State* state = State::create();
  state->runBinaryFromFile("../boot.bin");

  {
    yalpbind::YalpBind b(state);
    b.bind("square", square);
  }
  state->runFromString("(print (square 1111))");

  state->release();
  return 0;
}
