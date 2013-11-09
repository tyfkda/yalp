#include "macp.hh"
#include <iostream>

using namespace std;

int main() {
  cout << "sizeof(int) = " << sizeof(int) << endl;
  cout << "sizeof(long) = " << sizeof(long) << endl;
  cout << "sizeof(ptr) = " << sizeof(void*) << endl;

  macp::State* state = macp::State::create();
  delete state;
  return 0;
}
