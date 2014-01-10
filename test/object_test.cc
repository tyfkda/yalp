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
  Value a = Value(111);
  Value d = Value(222);
  Value cell = state_->cons(a, d);
  ASSERT_EQ(TT_CELL, cell.getType());
  Cell* p = static_cast<Cell*>(cell.toObject());
  ASSERT_TRUE(p->car().eq(a));
  ASSERT_TRUE(p->cdr().eq(d));
}
