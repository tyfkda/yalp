#include "gtest/gtest.h"
#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/util.hh"

using namespace yalp;

class StateTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    state_ = State::create();
  }

  virtual void TearDown() override {
    state_->release();
  }

  State* state_;
};

TEST_F(StateTest, Fixnum) {
  Value s = Value(123);
  ASSERT_TRUE(Value(123).eq(s));
}

TEST_F(StateTest, Symbol) {
  Value s = state_->intern("symbol");
  ASSERT_TRUE(state_->intern("symbol").eq(s));
  ASSERT_FALSE(state_->intern("otherSymbol").eq(s));

  ASSERT_FALSE(s.isObject());
  ASSERT_STREQ("symbol", s.toSymbol(state_)->c_str());
}

TEST_F(StateTest, Nil) {
  Value nil = state_->intern("nil");
  ASSERT_TRUE(nil.eq(Value::NIL));
}

TEST_F(StateTest, Cons) {
  Value v = state_->cons(Value(1), Value(2));
  ASSERT_EQ(TT_CELL, v.getType());
  ASSERT_TRUE(Value(1).eq(state_->car(v)));
  ASSERT_TRUE(Value(2).eq(state_->cdr(v)));
  ASSERT_TRUE(v.eq(v));
  ASSERT_TRUE(v.equal(v));

  Value v2 = state_->cons(Value(1), Value(2));
  ASSERT_FALSE(v.eq(v2));
  ASSERT_TRUE(v.equal(v2));

  Value v3 = state_->cons(Value(10), Value(20));
  ASSERT_FALSE(v.eq(v3));
  ASSERT_FALSE(v.equal(v3));
}

TEST_F(StateTest, Funcall) {
  Value args[] = { Value(1), Value(2), Value(3) };
  Value fn = state_->referGlobal(state_->intern("+"));
  Value result;
  ASSERT_TRUE(state_->funcall(fn, sizeof(args) / sizeof(*args), args, &result));
  ASSERT_EQ(TT_FIXNUM, result.getType());
  ASSERT_EQ(6, result.toFixnum());
}
