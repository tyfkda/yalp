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
  Value a = Value(1);
  Value b = Value(2);
  Value c = Value(3);

  Value s1 = list(state_, a);
  ASSERT_TRUE(state_->cons(a, Value::NIL).equal(s1));

  Value s2 = list(state_, a, b);
  ASSERT_TRUE(state_->cons(a, state_->cons(b, Value::NIL)).equal(s2));

  Value s3 = list(state_, Value(1), Value(2), Value(3));
  ASSERT_TRUE(state_->cons(a, state_->cons(b,  state_->cons(c, Value::NIL))).equal(s3));
}

TEST_F(UtilTest, Nreverse) {
  Value a = Value(1);
  Value b = Value(2);
  Value c = Value(3);
  Value d = Value(3);

  Value s = list(state_, a);
  Value reversed = nreverse(s);
  ASSERT_TRUE(state_->cons(a, Value::NIL).equal(reversed));

  Value s2 = list(state_, a, b, c);
  Value reversed2 = nreverse(s2);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, Value::NIL))).equal(reversed2));

  Value s3 = state_->cons(a, state_->cons(b,  state_->cons(c, d)));
  Value reversed3 = nreverse(s3);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, Value::NIL))).equal(reversed3));
}

TEST_F(UtilTest, length) {
  Value a = Value(1);
  ASSERT_EQ(0, length(Value::NIL));
  ASSERT_EQ(3, length(list(state_, a, a, a)));
  ASSERT_EQ(1, length(state_->cons(a, a)));
}
