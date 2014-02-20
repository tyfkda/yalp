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
    reader_ = NULL;
  }

  virtual void TearDown() override {
    if (reader_ != NULL)
      delete reader_;
    state_->release();
  }

  ErrorCode read(const char* str, Value* pValue) {
    StrStream stream(str);
    reader_ = new Reader(state_, &stream);
    return reader_->read(pValue);
  }

  State* state_;
  Reader* reader_;
};

TEST_F(ReadTest, Comment) {
  Value s;
  ASSERT_EQ(SUCCESS, read("(1 #| 2 |# 3)", &s));
  ASSERT_TRUE(list(state_, Value(1), Value(3)).equal(s));
}

TEST_F(ReadTest, BlockComment) {
  Value s;
  ASSERT_EQ(SUCCESS, read("(1 #| 2 #| 3 |# 4 |# 5)", &s));
  ASSERT_TRUE(list(state_, Value(1), Value(5)).equal(s));

  ASSERT_EQ(ILLEGAL_CHAR, read("(1 #| unterminated block comment", &s));
}

TEST_F(ReadTest, Eof) {
  Value s;
  ASSERT_EQ(END_OF_FILE, read("", &s));
}

TEST_F(ReadTest, Fixnum) {
  Value s;
  ASSERT_EQ(SUCCESS, read("123", &s));
  ASSERT_TRUE(Value(123).eq(s));

  ASSERT_EQ(SUCCESS, read("-123", &s));
  ASSERT_TRUE(Value(-123).eq(s));
}

TEST_F(ReadTest, Symbol) {
  Value s;
  ASSERT_EQ(SUCCESS, read("symbol", &s));
  ASSERT_TRUE(state_->intern("symbol").eq(s));
  ASSERT_EQ(SUCCESS, read("+=", &s));
  ASSERT_TRUE(state_->intern("+=").eq(s));
}

TEST_F(ReadTest, List) {
  Value s;
  ASSERT_EQ(SUCCESS, read("(123)", &s));
  ASSERT_TRUE(list(state_, Value(123)).equal(s));

  Value s2;
  ASSERT_EQ(SUCCESS, read("(1 2 3)", &s2));
  ASSERT_TRUE(list(state_,
                   Value(1),
                   Value(2),
                   Value(3)).equal(s2));

  Value s3;
  ASSERT_EQ(SUCCESS, read("(1 (2) 3)", &s3));
  ASSERT_TRUE(list(state_,
                   Value(1),
                   list(state_, Value(2)),
                   Value(3)).equal(s3));
}

TEST_F(ReadTest, DottedList) {
  Value s;
  ASSERT_EQ(DOT_AT_BASE, read(".", &s)) << "Dot is not a symbol";

  ASSERT_EQ(SUCCESS, read("(1 2 . 3)", &s));
  ASSERT_TRUE(state_->cons(Value(1),
                           state_->cons(Value(2),
                                        Value(3))).equal(s));
}

TEST_F(ReadTest, SharedStructure) {
  {
    Value s;
    ASSERT_EQ(SUCCESS, read("(#0=(a) #0#)", &s));
    ASSERT_EQ(TT_CELL, s.getType());
    Cell* cell = static_cast<Cell*>(s.toObject());
    ASSERT_EQ(TT_CELL, cell->cdr().getType());
    ASSERT_TRUE(cell->car().eq(static_cast<Cell*>(cell->cdr().toObject())->car()));
  }

  {
    Value s;
    ASSERT_EQ(SUCCESS, read("#0=(a . #0#)", &s));
    ASSERT_EQ(TT_CELL, s.getType());
    Cell* cell = static_cast<Cell*>(s.toObject());
    ASSERT_TRUE(s.eq(cell->cdr()));
  }
}

TEST_F(ReadTest, String) {
  Value s;
  ASSERT_EQ(SUCCESS, read("\"string\"", &s));
  ASSERT_FALSE(state_->string("string").eq(s));
  ASSERT_TRUE(state_->string("string").equal(s));

  ASSERT_EQ(SUCCESS, read("\"a b\\tc\\nd\"", &s));
  ASSERT_TRUE(state_->string("a b\tc\nd").equal(s));

  ASSERT_EQ(SUCCESS, read("\"'\\\"foobar\\\"'\"", &s));
  ASSERT_TRUE(state_->string("'\"foobar\"'").equal(s));

  ASSERT_EQ(SUCCESS, read("\"null\\0char\"", &s));
  ASSERT_TRUE(state_->string("null\0char", 9).equal(s));
  ASSERT_FALSE(state_->string("null").equal(s));
}

#ifndef DISABLE_FLONUM
TEST_F(ReadTest, Flonum) {
  Value s;
  Flonum f = static_cast<Flonum>(1.23);
  ASSERT_EQ(SUCCESS, read("1.23", &s));
  ASSERT_TRUE(s.getType() == TT_FLONUM);
  ASSERT_EQ(f, s.toFlonum(state_));

  ASSERT_EQ(SUCCESS, read("-1.23", &s));
  ASSERT_TRUE(state_->flonum(-f).equal(s));
}
#endif

TEST_F(ReadTest, Char) {
  Value s;
  ASSERT_EQ(SUCCESS, read("#\\A", &s));
  ASSERT_EQ('A', s.toCharacter());

  ASSERT_EQ(SUCCESS, read("#\\[", &s));
  ASSERT_EQ('[', s.toCharacter());

  ASSERT_EQ(SUCCESS, read("#\\space", &s));
  ASSERT_EQ(' ', s.toCharacter());

  ASSERT_EQ(SUCCESS, read("#\\nl", &s));  // #\newline, or #\nl
  ASSERT_EQ('\n', s.toCharacter());

  ASSERT_EQ(SUCCESS, read("#\\tab", &s));
  ASSERT_EQ('\t', s.toCharacter());

  ASSERT_EQ(SUCCESS, read("#\\x80", &s));
  ASSERT_EQ(0x80, s.toCharacter());

  ASSERT_EQ(SUCCESS, read("#\\あ", &s));  // "あ" = [0xe3, 0x81, 0x82] = [0b11100011, 0b10000001, 0b10000010] = 0b0011_0000_0100_0010
  ASSERT_EQ(0x3042, s.toCharacter());
}

TEST_F(ReadTest, Error) {
  Value s;
  ASSERT_EQ(NO_CLOSE_PAREN, read("(1\n (2\n (3)", &s));
  ASSERT_EQ(2, reader_->getLineNumber());
  ASSERT_EQ(EXTRA_CLOSE_PAREN, read("\n\n)", &s));
  ASSERT_EQ(3, reader_->getLineNumber());
  ASSERT_EQ(ILLEGAL_CHAR, read("(. 1)", &s));
  ASSERT_EQ(1, reader_->getLineNumber());
  ASSERT_EQ(NO_CLOSE_PAREN, read("(1 . 2\n 3)", &s));
  ASSERT_EQ(2, reader_->getLineNumber());
  ASSERT_EQ(NO_CLOSE_STRING, read("\"string", &s));
  ASSERT_EQ(1, reader_->getLineNumber());
}
