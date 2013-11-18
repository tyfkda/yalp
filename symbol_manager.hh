//=============================================================================
/// Symbol manager.
//=============================================================================

#ifndef _SYMBOL_MANAGER_HH_
#define _SYMBOL_MANAGER_HH_

#include <vector>

namespace yalp {

class Symbol;

class SymbolManager {
public:
  SymbolManager();
  ~SymbolManager();

  Symbol* intern(const char* name);

private:
  static const char* copyString(const char* name);

  std::vector<Symbol*> table_;
};

}  // namespace yalp

#endif
