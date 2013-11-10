//=============================================================================
/// Symbol manager.
//=============================================================================

#ifndef _SYMBOL_MANAGER_HH_
#define _SYMBOL_MANAGER_HH_

#include "yalp.hh"
#include <vector>

namespace yalp {

class SymbolManager {
public:
  SymbolManager();
  ~SymbolManager();

  Svalue intern(const char* name);

private:
  static const char* copyString(const char* name);

  std::vector<Svalue> table_;
};

}  // namespace yalp

#endif
