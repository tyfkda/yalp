#include "read.hh"
#include "yalp.hh"
#include <fstream>
#include <iostream>

using namespace std;
using namespace yalp;

void runBinary(State* state, std::istream& istrm) {
  Reader reader(state, istrm);
  
  Svalue bin;
  ReadError err;
  while ((err = reader.read(&bin)) == SUCCESS) {
    state->runBinary(bin);
  }
  if (err != END_OF_FILE)
    cerr << "Read error: " << err << endl;
}

int main(int argc, char* argv[]) {
  State* state = State::create();

  if (argc < 2) {
    runBinary(state, cin);
  } else {
    std::ifstream strm(argv[1]);
    runBinary(state, strm);
  }

  delete state;
  return 0;
}
