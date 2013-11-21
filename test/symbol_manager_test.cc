#include "gtest/gtest.h"
#include "../src/symbol_manager.hh"

using namespace yalp;

class SymbolManagerTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    symbolManager_ = SymbolManager::create(Allocator::getDefaultAllocator());
  }

  virtual void TearDown() override {
    symbolManager_->release();
  }

  SymbolManager* symbolManager_;
};

TEST_F(SymbolManagerTest, Fixnum) {
  Symbol* symbol1 = symbolManager_->intern("symbol");
  Symbol* symbol2 = symbolManager_->intern("symbol");
  ASSERT_EQ(symbol1, symbol2);
}
