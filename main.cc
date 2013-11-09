#include "macp.hh"

int main() {
  macp::State* state = macp::State::create();
  delete state;
  return 0;
}
