#include "gtest/gtest.h"
#include "yalp/read.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"

using namespace yalp;

class ReadTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    state_ = State::create();
  }

  virtual void TearDown() override {
    state_->release();
  }

  ErrorCode read(const char* str, Svalue* pValue) {
    StrStream stream(str);
    Reader reader(state_, &stream);
    return reader.read(pValue);
  }

  State* state_;
};

TEST_F(ReadTest, LineComment) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read(" ; Line comment\n 123", &s));
  ASSERT_TRUE(state_->fixnumValue(123).eq(s));
}

TEST_F(ReadTest, Eof) {
  Svalue s;
  ASSERT_EQ(END_OF_FILE, read("", &s));
}

TEST_F(ReadTest, Fixnum) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read("123", &s));
  ASSERT_TRUE(state_->fixnumValue(123).eq(s));

  ASSERT_EQ(SUCCESS, read("-123", &s));
  ASSERT_TRUE(state_->fixnumValue(-123).eq(s));
}

TEST_F(ReadTest, Symbol) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read("symbol", &s));
  ASSERT_TRUE(state_->intern("symbol").eq(s));
  ASSERT_EQ(SUCCESS, read("+=", &s));
  ASSERT_TRUE(state_->intern("+=").eq(s));
}

TEST_F(ReadTest, List) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read("(123)", &s));
  ASSERT_TRUE(list(state_, state_->fixnumValue(123)).equal(s));

  Svalue s2;
  ASSERT_EQ(SUCCESS, read("(1 2 3)", &s2));
  ASSERT_TRUE(list(state_,
                   state_->fixnumValue(1),
                   state_->fixnumValue(2),
                   state_->fixnumValue(3)).equal(s2));

  Svalue s3;
  ASSERT_EQ(SUCCESS, read("(1 (2) 3)", &s3));
  ASSERT_TRUE(list(state_,
                   state_->fixnumValue(1),
                   list(state_, state_->fixnumValue(2)),
                   state_->fixnumValue(3)).equal(s3));
}

TEST_F(ReadTest, DottedList) {
  Svalue s;
  ASSERT_EQ(DOT_AT_BASE, read(".", &s)) << "Dot is not a symbol";

  ASSERT_EQ(SUCCESS, read("(1 2 . 3)", &s));
  ASSERT_TRUE(state_->cons(state_->fixnumValue(1),
                           state_->cons(state_->fixnumValue(2),
                                        state_->fixnumValue(3))).equal(s));
}

TEST_F(ReadTest, Quote) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read("'(x y z)", &s));
  ASSERT_TRUE(list(state_,
                   state_->intern("quote"),
                   list(state_,
                        state_->intern("x"),
                        state_->intern("y"),
                        state_->intern("z"))).equal(s));
}

TEST_F(ReadTest, quasiquote) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read("`x", &s));
  ASSERT_TRUE(list(state_,
                   state_->intern("quasiquote"),
                   state_->intern("x")).equal(s));

  ASSERT_EQ(SUCCESS, read(",x", &s));
  ASSERT_TRUE(list(state_,
                   state_->intern("unquote"),
                   state_->intern("x")).equal(s));

  ASSERT_EQ(SUCCESS, read(",@x", &s));
  ASSERT_TRUE(list(state_,
                   state_->intern("unquote-splicing"),
                   state_->intern("x")).equal(s));
}

TEST_F(ReadTest, SharedStructure) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read("(#0=(a) #0#)", &s));
  ASSERT_EQ(TT_CELL, s.getType());
  Cell* cell = static_cast<Cell*>(s.toObject());
  ASSERT_EQ(TT_CELL, cell->cdr().getType());
  ASSERT_TRUE(cell->car().eq(static_cast<Cell*>(cell->cdr().toObject())->car()));
}

TEST_F(ReadTest, String) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read("\"string\"", &s));
  ASSERT_FALSE(state_->stringValue("string").eq(s));
  ASSERT_TRUE(state_->stringValue("string").equal(s));

  ASSERT_EQ(SUCCESS, read("\"a b\\tc\\nd\"", &s));
  ASSERT_TRUE(state_->stringValue("a b\tc\nd").equal(s));

  ASSERT_EQ(SUCCESS, read("\"'\\\"foobar\\\"'\"", &s));
  ASSERT_TRUE(state_->stringValue("'\"foobar\"'").equal(s));

  ASSERT_EQ(SUCCESS, read("\"null\\0char\"", &s));
  ASSERT_TRUE(state_->stringValue("null\0char", 9).equal(s));
  ASSERT_FALSE(state_->stringValue("null").equal(s));
}

TEST_F(ReadTest, Float) {
  Svalue s;
  Flonum f = static_cast<Flonum>(1.23);
  ASSERT_EQ(SUCCESS, read("1.23", &s));
  ASSERT_TRUE(s.getType() == TT_FLONUM);
  ASSERT_EQ(f, s.toFlonum(state_));

  ASSERT_EQ(SUCCESS, read("-1.23", &s));
  ASSERT_TRUE(state_->flonumValue(-f).equal(s));
}

TEST_F(ReadTest, Char) {
  Svalue s;
  ASSERT_EQ(SUCCESS, read("#\\A", &s));
  ASSERT_TRUE(state_->fixnumValue(65).eq(s));

  ASSERT_EQ(SUCCESS, read("#\\[", &s));
  ASSERT_TRUE(state_->fixnumValue('[').eq(s));

  ASSERT_EQ(SUCCESS, read("#\\space", &s));
  ASSERT_TRUE(state_->fixnumValue(' ').eq(s));

  ASSERT_EQ(SUCCESS, read("#\\nl", &s));  // #\newline, or #\nl
  ASSERT_TRUE(state_->fixnumValue('\n').eq(s));

  ASSERT_EQ(SUCCESS, read("#\\tab", &s));
  ASSERT_TRUE(state_->fixnumValue('\t').eq(s));
}

TEST_F(ReadTest, Error) {
  Svalue s;
  ASSERT_EQ(NO_CLOSE_PAREN, read("(1 (2) 3", &s));
  ASSERT_EQ(EXTRA_CLOSE_PAREN, read(")", &s));
  ASSERT_EQ(ILLEGAL_CHAR, read("(. 1)", &s));
  ASSERT_EQ(NO_CLOSE_PAREN, read("(1 . 2 3)", &s));
  ASSERT_EQ(NO_CLOSE_STRING, read("\"string", &s));
}
