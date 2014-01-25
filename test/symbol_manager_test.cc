#include "gtest/gtest.h"
#include "symbol_manager.hh"

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

TEST_F(SymbolManagerTest, InternReturnsSameObjectForSameSymbolName) {
  SymbolId symbol1 = symbolManager_->intern("symbol");
  SymbolId symbol2 = symbolManager_->intern("symbol");
  ASSERT_EQ(symbol1, symbol2);
}
