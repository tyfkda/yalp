//=============================================================================
/// Symbol manager.
//=============================================================================

#ifndef _SYMBOL_MANAGER_HH_
#define _SYMBOL_MANAGER_HH_

#include <vector>

namespace yalp {

class Allocator;
class Symbol;

class SymbolManager {
public:
  static SymbolManager* create(Allocator* allocator);
  // Delete.
  void release();

  // Create symbol from c-string.
  Symbol* intern(const char* name);

  // Generate new symbol.
  Symbol* gensym();

private:
  SymbolManager(Allocator* allocator);
  ~SymbolManager();
  Symbol* generate(const char* name);
  const char* copyString(const char* name);

  Allocator* allocator_;
  std::vector<Symbol*> table_;
  int gensymIndex_;
};

}  // namespace yalp

#endif
