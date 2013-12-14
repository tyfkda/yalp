#include "gtest/gtest.h"
#include "yalp/object.hh"

using namespace yalp;

class ObjectTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    state_ = State::create();
  }

  virtual void TearDown() override {
    state_->release();
  }

  State* state_;
};

TEST_F(ObjectTest, cell) {
  Svalue a = state_->fixnumValue(111);
  Svalue d = state_->fixnumValue(222);
  Svalue cell = state_->cons(a, d);
  ASSERT_EQ(TT_CELL, cell.getType());
  ASSERT_TRUE(state_->car(cell).eq(a));
  ASSERT_TRUE(state_->cdr(cell).eq(d));
}
