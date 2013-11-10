#include "read.hh"
#include "yalp.hh"
#include <iostream>

using namespace std;
using namespace yalp;

int main(int argc, char* argv[]) {
  cout << "sizeof(int) = " << sizeof(int) << endl;
  cout << "sizeof(long) = " << sizeof(long) << endl;
  cout << "sizeof(ptr) = " << sizeof(void*) << endl;
  cout << "sizeof(Svalue) = " << sizeof(Svalue) << endl;
  cout << "sizeof(Sobject) = " << sizeof(Sobject) << endl;
  cout << "sizeof(Cell) = " << sizeof(Cell) << endl;

  State* state = State::create();

  if (argc >= 2) {
    Svalue v;
    readFromFile(state, argv[1], &v);
    cout << "Code: " << v << endl;
    cout << "Run: " << state->runBinary(v) << endl;
  } else {
    Svalue v = list3(state, state->fixnumValue(1), state->fixnumValue(2), state->fixnumValue(3));
    cout << v << endl;
  }

  delete state;
  return 0;
}
