#include "gtest/gtest.h"
#include "symbol_manager.hh"
#include "yalp/object.hh"

using namespace yalp;

class SymbolManagerTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    allocator_ = Allocator::create(getDefaultAllocFunc(), NULL);
    symbolManager_ = SymbolManager::create(allocator_);
  }

  virtual void TearDown() override {
    symbolManager_->release();
    allocator_->release();
  }

  SymbolManager* symbolManager_;
  Allocator* allocator_;
};

TEST_F(SymbolManagerTest, Fixnum) {
  SymbolId symbol1 = symbolManager_->intern("symbol");
  SymbolId symbol2 = symbolManager_->intern("symbol");
  ASSERT_EQ(symbol1, symbol2);
}

TEST_F(SymbolManagerTest, Gensym) {
  static const char NAME[] = "#G:1";
  SymbolId symbol1 = symbolManager_->gensym();  // This is the first gensym call.
  ASSERT_STREQ(NAME, symbolManager_->get(symbol1)->c_str());
  SymbolId symbol2 = symbolManager_->intern(NAME);
  ASSERT_NE(symbol1, symbol2);
}
