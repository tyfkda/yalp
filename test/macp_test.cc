#include "gtest/gtest.h"
#include "macp.hh"

using namespace macp;

class MacpTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    state_ = State::create();
  }

  virtual void TearDown() override {
    delete state_;
  }

  State* state_;
};

TEST_F(MacpTest, Read) {
  Svalue s = state_->readString("123");
  ASSERT_TRUE(State::eq(state_->fixnumValue(123), s));
}

TEST_F(MacpTest, Cons) {
  Svalue v = state_->cons(state_->fixnumValue(1), state_->fixnumValue(2));
  ASSERT_EQ(TT_CELL, state_->getType(v));
  Scell* cell = static_cast<Scell*>(state_->toObject(v));
  ASSERT_EQ(1, cell->car());
  ASSERT_EQ(2, cell->cdr());
}
