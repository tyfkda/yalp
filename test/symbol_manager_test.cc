#include "gtest/gtest.h"
#include "symbol_manager.hh"

using namespace yalp;

class SymbolManagerTest : public ::testing::Test {
protected:
  SymbolManager symbolManager_;
};

TEST_F(SymbolManagerTest, Fixnum) {
  Svalue symbol = symbolManager_.intern("symbol");
  ASSERT_TRUE(symbolManager_.intern("symbol").eq(symbol));
}
