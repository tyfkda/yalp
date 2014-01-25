#include "gtest/gtest.h"
#include "yalp/stream.hh"
#include "allocator.hh"

using namespace yalp;

class StreamTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    allocator_ = Allocator::create(getDefaultAllocFunc(), NULL);
  }

  virtual void TearDown() override {
    allocator_->release();
  }

  Allocator* allocator_;
};

TEST_F(StreamTest, testStrOStream) {
  StrOStream stream(allocator_);
  stream.write("foo");
  stream.write("bar");
  stream.write("baz");
  ASSERT_EQ(9, stream.getLength());
  ASSERT_STREQ("foobarbaz", stream.getString());
}
