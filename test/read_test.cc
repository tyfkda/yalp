#include "gtest/gtest.h"
#include "read.hh"

using namespace macp;

class ReadTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    state_ = State::create();
  }

  virtual void TearDown() override {
    delete state_;
  }

  State* state_;
};

TEST_F(ReadTest, LineComment) {
  Svalue s;
  ASSERT_EQ(SUCCESS, readFromString(state_, " ; Line comment\n 123", &s));
  ASSERT_TRUE(state_->fixnumValue(123).eq(s)) << s;
}

TEST_F(ReadTest, Fixnum) {
  Svalue s;
  ASSERT_EQ(SUCCESS, readFromString(state_, "123", &s));
  ASSERT_TRUE(state_->fixnumValue(123).eq(s)) << s;
}

TEST_F(ReadTest, Symbol) {
  Svalue s;
  ASSERT_EQ(SUCCESS, readFromString(state_, "symbol", &s));
  ASSERT_TRUE(state_->intern("symbol").eq(s)) << s;
}

TEST_F(ReadTest, List) {
  Svalue s;
  ASSERT_EQ(SUCCESS, readFromString(state_, "(123)", &s));
  ASSERT_TRUE(list1(state_, state_->fixnumValue(123)).equal(s)) << s;

  Svalue s2;
  ASSERT_EQ(SUCCESS, readFromString(state_, "(1 2 3)", &s2));
  ASSERT_TRUE(list3(state_,
                    state_->fixnumValue(1),
                    state_->fixnumValue(2),
                    state_->fixnumValue(3)).equal(s2)) << s2;

  Svalue s3;
  ASSERT_EQ(SUCCESS, readFromString(state_, "(1 (2) 3)", &s3));
  ASSERT_TRUE(list3(state_,
                    state_->fixnumValue(1),
                    list1(state_, state_->fixnumValue(2)),
                    state_->fixnumValue(3)).equal(s3)) << s3;
}

TEST_F(ReadTest, SharedStructure) {
  Svalue s;
  ASSERT_EQ(SUCCESS, readFromString(state_, "(#0=(a) #0#)", &s));
  ASSERT_EQ(TT_CELL, s.getType());
  Cell* cell = static_cast<Cell*>(s.toObject());
  ASSERT_EQ(TT_CELL, cell->cdr().getType());
  ASSERT_TRUE(cell->car().eq(static_cast<Cell*>(cell->cdr().toObject())->car())) << s;
}

TEST_F(ReadTest, Error) {
  Svalue s;
  ASSERT_EQ(NO_CLOSE_PAREN, readFromString(state_, "(1 (2) 3", &s));
}
