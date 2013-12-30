#include "yalp.hh"
#include <iostream>

using yalp::State;
using yalp::Value;
using namespace std;

int main() {
  State* state = State::create();
  state->runBinaryFromFile("../boot.bin");

  Value fn = state->referGlobal("+");
  Value args[] = { Value(1), Value(2) };
  Value result;
  if (state->funcall(fn, 2, args, &result)) {
    cout << "Result: " << result.toFixnum() << endl;
  } else {
    cout << "Funcall error" << endl;
  }

  state->release();
  return 0;
}
