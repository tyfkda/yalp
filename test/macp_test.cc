#include "gtest/gtest.h"
#include "macp.hh"

using namespace macp;

TEST(MacpTest, Create) {
  State* state = State::create();
  delete state;
}
