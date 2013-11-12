#include "read.hh"
#include "yalp.hh"
#include <iostream>

using namespace std;
using namespace yalp;

int main(int argc, char* argv[]) {
  State* state = State::create();

  if (argc >= 2) {
    Svalue v;
    ReadError err = readFromFile(state, argv[1], &v);
    if (err != SUCCESS) {
      cerr << "Read error: " << err << endl;
    } else {
      cout << state->runBinary(v) << endl;
    }
  } else {
    Svalue v = list3(state, state->fixnumValue(1), state->fixnumValue(2), state->fixnumValue(3));
    cout << v << endl;
  }

  delete state;
  return 0;
}
