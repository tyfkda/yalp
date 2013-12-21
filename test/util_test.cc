#include "gtest/gtest.h"
#include "yalp/util.hh"
#include "yalp.hh"

using namespace yalp;

class UtilTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    state_ = State::create();
  }

  virtual void TearDown() override {
    state_->release();
  }

  State* state_;
};

TEST_F(UtilTest, ListFunctions) {
  Svalue a = state_->fixnumValue(1);
  Svalue b = state_->fixnumValue(2);
  Svalue c = state_->fixnumValue(3);

  Svalue s1 = list(state_, a);
  ASSERT_TRUE(state_->cons(a, Svalue::NIL).equal(s1));

  Svalue s2 = list(state_, a, b);
  ASSERT_TRUE(state_->cons(a, state_->cons(b, Svalue::NIL)).equal(s2));

  Svalue s3 = list(state_, state_->fixnumValue(1), state_->fixnumValue(2), state_->fixnumValue(3));
  ASSERT_TRUE(state_->cons(a, state_->cons(b,  state_->cons(c, Svalue::NIL))).equal(s3));
}

TEST_F(UtilTest, Nreverse) {
  Svalue a = state_->fixnumValue(1);
  Svalue b = state_->fixnumValue(2);
  Svalue c = state_->fixnumValue(3);
  Svalue d = state_->fixnumValue(3);

  Svalue s = list(state_, a);
  Svalue reversed = nreverse(s);
  ASSERT_TRUE(state_->cons(a, Svalue::NIL).equal(reversed));

  Svalue s2 = list(state_, a, b, c);
  Svalue reversed2 = nreverse(s2);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, Svalue::NIL))).equal(reversed2));

  Svalue s3 = state_->cons(a, state_->cons(b,  state_->cons(c, d)));
  Svalue reversed3 = nreverse(s3);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, Svalue::NIL))).equal(reversed3));
}

TEST_F(UtilTest, length) {
  Svalue a = state_->fixnumValue(1);
  ASSERT_EQ(0, length(Svalue::NIL));
  ASSERT_EQ(3, length(list(state_, a, a, a)));
  ASSERT_EQ(1, length(state_->cons(a, a)));
}
