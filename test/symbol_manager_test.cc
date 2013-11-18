#include "gtest/gtest.h"
#include "symbol_manager.hh"

using namespace yalp;

class SymbolManagerTest : public ::testing::Test {
protected:
  SymbolManager symbolManager_;
};

TEST_F(SymbolManagerTest, Fixnum) {
  Symbol* symbol1 = symbolManager_.intern("symbol");
  Symbol* symbol2 = symbolManager_.intern("symbol");
  ASSERT_EQ(symbol1, symbol2);
}
