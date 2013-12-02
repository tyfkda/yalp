#include "gtest/gtest.h"
#include "yalp.hh"
#include "yalp/object.hh"

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
  Svalue s = state_->fixnumValue(123);
  ASSERT_TRUE(state_->fixnumValue(123).eq(s));
}

TEST_F(StateTest, Symbol) {
  Svalue s = state_->intern("symbol");
  ASSERT_TRUE(state_->intern("symbol").eq(s));
  ASSERT_FALSE(state_->intern("otherSymbol").eq(s));

  ASSERT_FALSE(s.isObject());
  ASSERT_STREQ("symbol", s.toSymbol(state_)->c_str());
}

TEST_F(StateTest, Cons) {
  Svalue v = state_->cons(state_->fixnumValue(1), state_->fixnumValue(2));
  ASSERT_EQ(TT_CELL, v.getType());
  ASSERT_TRUE(state_->fixnumValue(1).eq(state_->car(v)));
  ASSERT_TRUE(state_->fixnumValue(2).eq(state_->cdr(v)));
  ASSERT_TRUE(v.eq(v));
  ASSERT_TRUE(v.equal(v));

  Svalue v2 = state_->cons(state_->fixnumValue(1), state_->fixnumValue(2));
  ASSERT_FALSE(v.eq(v2));
  ASSERT_TRUE(v.equal(v2));

  Svalue v3 = state_->cons(state_->fixnumValue(10), state_->fixnumValue(20));
  ASSERT_FALSE(v.eq(v3));
  ASSERT_FALSE(v.equal(v3));
}

TEST_F(StateTest, Funcall) {
  Svalue args[] = { state_->fixnumValue(1), state_->fixnumValue(2), state_->fixnumValue(3) };
  Svalue fn = state_->referGlobal(state_->intern("+"));
  Svalue result = state_->funcall(fn, sizeof(args) / sizeof(*args), args);
  ASSERT_EQ(TT_FIXNUM, result.getType());
  ASSERT_EQ(6, result.toFixnum());
}

TEST_F(StateTest, ListFunctions) {
  Svalue a = state_->fixnumValue(1);
  Svalue b = state_->fixnumValue(2);
  Svalue c = state_->fixnumValue(3);

  Svalue s1 = list(state_, a);
  ASSERT_TRUE(state_->cons(a, state_->nil()).equal(s1));

  Svalue s2 = list(state_, a, b);
  ASSERT_TRUE(state_->cons(a, state_->cons(b, state_->nil())).equal(s2));

  Svalue s3 = list(state_, state_->fixnumValue(1), state_->fixnumValue(2), state_->fixnumValue(3));
  ASSERT_TRUE(state_->cons(a, state_->cons(b,  state_->cons(c, state_->nil()))).equal(s3));
}

TEST_F(StateTest, Nreverse) {
  Svalue a = state_->fixnumValue(1);
  Svalue b = state_->fixnumValue(2);
  Svalue c = state_->fixnumValue(3);
  Svalue d = state_->fixnumValue(3);

  Svalue s = list(state_, a);
  Svalue reversed = nreverse(state_, s);
  ASSERT_TRUE(state_->cons(a, state_->nil()).equal(reversed));

  Svalue s2 = list(state_, a, b, c);
  Svalue reversed2 = nreverse(state_, s2);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, state_->nil()))).equal(reversed2));

  Svalue s3 = state_->cons(a, state_->cons(b,  state_->cons(c, d)));
  Svalue reversed3 = nreverse(state_, s3);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, state_->nil()))).equal(reversed3));
}

TEST_F(StateTest, length) {
  Svalue a = state_->fixnumValue(1);
  ASSERT_EQ(0, length(state_, state_->nil()));
  ASSERT_EQ(3, length(state_, list(state_, a, a, a)));
  ASSERT_EQ(1, length(state_, state_->cons(a, a)));
}
