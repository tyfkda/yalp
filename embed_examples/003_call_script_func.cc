#include "yalp.hh"
#include <iostream>

using yalp::State;
using yalp::Value;
using yalp::bootBinaryData;
using namespace std;

int main() {
  State* state = State::create();
  state->runBinaryFromString(bootBinaryData);

  Value fn = state->referGlobal("+");
  Value args[] = { Value(1), Value(2) };
  Value result;
  if (state->funcall(fn, 2, args, &result)) {
    cout << result.toFixnum() << endl;
  } else {
    cout << "Funcall error" << endl;
  }

  state->release();
  return 0;
}
