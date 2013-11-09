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

TEST_F(MacpTest, Fixnum) {
  Svalue s = state_->fixnumValue(123);
  ASSERT_TRUE(state_->fixnumValue(123).eq(s)) << s;
}

TEST_F(MacpTest, Symbol) {
  Svalue s = state_->intern("symbol");
  ASSERT_TRUE(state_->intern("symbol").eq(s)) << s;
  ASSERT_FALSE(state_->intern("otherSymbol").eq(s)) << s;
}

TEST_F(MacpTest, Cons) {
  Svalue v = state_->cons(state_->fixnumValue(1), state_->fixnumValue(2));
  ASSERT_EQ(TT_CELL, v.getType());
  Cell* cell = static_cast<Cell*>(v.toObject());
  ASSERT_TRUE(state_->fixnumValue(1).eq(cell->car())) << v;
  ASSERT_TRUE(state_->fixnumValue(2).eq(cell->cdr())) << v;
  ASSERT_TRUE(v.eq(v)) << v;
  ASSERT_TRUE(v.equal(v)) << v;

  Svalue v2 = state_->cons(state_->fixnumValue(1), state_->fixnumValue(2));
  ASSERT_FALSE(v.eq(v2));
  ASSERT_TRUE(v.equal(v2));

  Svalue v3 = state_->cons(state_->fixnumValue(10), state_->fixnumValue(20));
  ASSERT_FALSE(v.eq(v3));
  ASSERT_FALSE(v.equal(v3));
}

TEST_F(MacpTest, Read) {
  Svalue s = state_->readString("123");
  ASSERT_TRUE(state_->fixnumValue(123).eq(s));
}
