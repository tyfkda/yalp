#include "gtest/gtest.h"
#include "symbol_manager.hh"

using namespace yalp;

class SymbolManagerTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    allocator_ = new Allocator(NULL, getDefaultAllocFunc());
    symbolManager_ = SymbolManager::create(allocator_);
  }

  virtual void TearDown() override {
    symbolManager_->release();
    delete allocator_;
  }

  SymbolManager* symbolManager_;
  Allocator* allocator_;
};

TEST_F(SymbolManagerTest, Fixnum) {
  Symbol* symbol1 = symbolManager_->intern("symbol");
  Symbol* symbol2 = symbolManager_->intern("symbol");
  ASSERT_EQ(symbol1, symbol2);
}
