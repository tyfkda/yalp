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

TEST_F(UtilTest, ReverseBang) {
  Value a = Value(1);
  Value b = Value(2);
  Value c = Value(3);
  Value d = Value(3);

  Value s = list(state_, a);
  Value reversed = reverseBang(s);
  ASSERT_TRUE(state_->cons(a, Value::NIL).equal(reversed));

  Value s2 = list(state_, a, b, c);
  Value reversed2 = reverseBang(s2);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, Value::NIL))).equal(reversed2));

  Value s3 = state_->cons(a, state_->cons(b,  state_->cons(c, d)));
  Value reversed3 = reverseBang(s3);
  ASSERT_TRUE(state_->cons(c, state_->cons(b,  state_->cons(a, Value::NIL))).equal(reversed3));
}

TEST_F(UtilTest, length) {
  Value a = Value(1);
  ASSERT_EQ(0, length(Value::NIL));
  ASSERT_EQ(3, length(list(state_, a, a, a)));
  ASSERT_EQ(1, length(state_->cons(a, a)));
}

TEST_F(UtilTest, utf8ToUnicode) {
  unsigned char a[] = "あ";  // "あ" = [0xe3, 0x81, 0x82] = [0b11100011, 0b10000001, 0b10000010] = 0b0011_0000_0100_0010
  unsigned char* p = a;
  ASSERT_EQ(0x3042, utf8ToUnicode(&p));
  ASSERT_EQ(3, p - a);
}

TEST_F(UtilTest, unicodeToUtf8) {
  unsigned char buffer[8];
  ASSERT_EQ(3, unicodeToUtf8(0x3042, buffer));
  ASSERT_EQ(0xe3, buffer[0]);
  ASSERT_EQ(0x81, buffer[1]);
  ASSERT_EQ(0x82, buffer[2]);
}
