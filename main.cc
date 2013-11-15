#include "read.hh"
#include "yalp.hh"
#include <fstream>
#include <iostream>

using namespace std;
using namespace yalp;

int main(int argc, char* argv[]) {
  State* state = State::create();

  if (argc >= 2) {
    std::ifstream strm(argv[1]);
    Reader reader(state, strm);

    Svalue bin;
    ReadError err;
    while ((err = reader.read(&bin)) == SUCCESS) {
      state->runBinary(bin);
    }
    if (err != END_OF_FILE)
      cerr << "Read error: " << err << endl;
  } else {
    Svalue v = list3(state, state->fixnumValue(1), state->fixnumValue(2), state->fixnumValue(3));
    cout << v << endl;
  }

  delete state;
  return 0;
}
