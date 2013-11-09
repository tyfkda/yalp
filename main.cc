#include "macp.hh"
#include <iostream>

using namespace std;
using namespace macp;

int main() {
  cout << "sizeof(int) = " << sizeof(int) << endl;
  cout << "sizeof(long) = " << sizeof(long) << endl;
  cout << "sizeof(ptr) = " << sizeof(void*) << endl;
  cout << "sizeof(Svalue) = " << sizeof(Svalue) << endl;
  cout << "sizeof(Sobject) = " << sizeof(Sobject) << endl;
  cout << "sizeof(Cell) = " << sizeof(Cell) << endl;

  State* state = State::create();

  Svalue v = list3(state, state->fixnumValue(1), state->fixnumValue(2), state->fixnumValue(3));
  cout << v << endl;

  delete state;
  return 0;
}
