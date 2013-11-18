//=============================================================================
/// Symbol manager.
//=============================================================================

#ifndef _SYMBOL_MANAGER_HH_
#define _SYMBOL_MANAGER_HH_

#include "mem.hh"
#include <vector>

namespace yalp {

class Symbol;

class SymbolManager {
public:
  static SymbolManager* create(Allocator* allocator);
  // Delete.
  void release();

  Symbol* intern(const char* name);

private:
  SymbolManager(Allocator* allocator);
  ~SymbolManager();
  const char* copyString(const char* name);

  Allocator* allocator_;
  std::vector<Symbol*> table_;
};

}  // namespace yalp

#endif
