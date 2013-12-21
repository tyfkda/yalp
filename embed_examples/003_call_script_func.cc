#include "yalp.hh"
#include <iostream>

using yalp::State;
using yalp::Svalue;
using namespace std;

int main() {
  State* state = State::create();
  state->runBinaryFromFile("../boot.bin");

  Svalue fn = state->referGlobal("+");
  Svalue args[] = { state->fixnumValue(1), state->fixnumValue(2) };
  Svalue result;
  if (state->funcall(fn, 2, args, &result)) {
    cout << "Result: " << result.toFixnum() << endl;
  } else {
    cout << "Funcall error" << endl;
  }

  state->release();
  return 0;
}
