#include "gtest/gtest.h"
#include "yalp.hh"

using namespace yalp;

class YalpTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    state_ = State::create();
  }

  virtual void TearDown() override {
    delete state_;
  }

  State* state_;
};

TEST_F(YalpTest, Fixnum) {
  Svalue s = state_->fixnumValue(123);
  ASSERT_TRUE(state_->fixnumValue(123).eq(s)) << s;
}

TEST_F(YalpTest, Symbol) {
  Svalue s = state_->intern("symbol");
  ASSERT_TRUE(state_->intern("symbol").eq(s)) << s;
  ASSERT_FALSE(state_->intern("otherSymbol").eq(s)) << s;
}

TEST_F(YalpTest, Cons) {
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

TEST_F(YalpTest, ListFunctions) {
  Svalue a = state_->fixnumValue(1);
  Svalue b = state_->fixnumValue(2);
  Svalue c = state_->fixnumValue(3);

  Svalue s1 = list1(state_, a);
  ASSERT_TRUE(state_->cons(a, state_->nil()).equal(s1));

  Svalue s2 = list2(state_, a, b);
  ASSERT_TRUE(state_->cons(a, state_->cons(b, state_->nil())).equal(s2));

  Svalue s3 = list3(state_, state_->fixnumValue(1), state_->fixnumValue(2), state_->fixnumValue(3));
  ASSERT_TRUE(state_->cons(a, state_->cons(b,  state_->cons(c, state_->nil()))).equal(s3));
}

TEST_F(YalpTest, Nreverse) {
  Svalue a = state_->fixnumValue(1);
  Svalue b = state_->fixnumValue(2);
  Svalue c = state_->fixnumValue(3);
  Svalue d = state_->fixnumValue(3);

  Svalue s = list1(state_, a);
  Svalue reversed = nreverse(state_, s);
  ASSERT_TRUE(state_->cons(a, state_->nil()).equal(reversed)) << reversed;

  Svalue s2 = list3(state_, a, b, c);
  Svalue reversed2 = nreverse(state_, s2);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, state_->nil()))).equal(reversed2)) << reversed2;

  Svalue s3 = state_->cons(a, state_->cons(b,  state_->cons(c, d)));
  Svalue reversed3 = nreverse(state_, s3);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, state_->nil()))).equal(reversed3)) << reversed3;
}
