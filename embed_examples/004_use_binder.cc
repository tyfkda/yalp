#include "yalp.hh"
#include "yalp/binder.hh"
#include "yalp/stream.hh"

#include <iostream>
#include <string>

using yalp::FileStream;
using yalp::Fixnum;
using yalp::State;
using yalp::Value;

int square(int x) {
  return x * x;
}

std::string emphasis(const std::string& str) {
  return "** " + str + " **";
}

int main() {
  State* state = State::create();
  state->runBinaryFromFile("../boot.bin");

  {
    yalp::bind::Binder b(state);
    b.bind("square", square);
    b.bind("atoi", atoi);
    b.bind("emphasis", emphasis);
  }
  state->runFromString("(print (square 1111))");
  state->runFromString("(print (atoi \"123\"))");
  state->runFromString("(print (emphasis \"Foo bar baz\"))");

  state->release();
  return 0;
}
