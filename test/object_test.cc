#include "gtest/gtest.h"
#include "object.hh"

using namespace yalp;

class CellForTest : public Cell {
public:
  CellForTest(Svalue a, Svalue d) : Cell(a, d) {}
};

class ValueTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    state_ = State::create();
  }

  virtual void TearDown() override {
    state_->release();
  }

  State* state_;
};

TEST_F(ValueTest, cell) {
  Svalue a = state_->fixnumValue(111);
  Svalue d = state_->fixnumValue(222);
  CellForTest cell(a, d);
  ASSERT_EQ(TT_CELL, cell.getType());
  ASSERT_TRUE(cell.car().eq(a));
  ASSERT_TRUE(cell.cdr().eq(d));
}
