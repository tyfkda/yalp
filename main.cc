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
  cout << "sizeof(Scell) = " << sizeof(Scell) << endl;

  State* state = State::create();
  delete state;
  return 0;
}
