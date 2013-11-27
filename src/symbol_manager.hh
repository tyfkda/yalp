//=============================================================================
/// Symbol manager.
//=============================================================================

#ifndef _SYMBOL_MANAGER_HH_
#define _SYMBOL_MANAGER_HH_

#include "hash_table.hh"

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

  void reportDebugInfo() const;

private:
  struct HashPolicy {
    static unsigned int hash(const char* a);
    static bool equal(const char* a, const char* b);
  };

  SymbolManager(Allocator* allocator);
  ~SymbolManager();
  Symbol* generate(const char* name);
  const char* copyString(const char* name);

  Allocator* allocator_;
  HashTable<const char*, Symbol*, HashPolicy> table_;
  int gensymIndex_;
};

}  // namespace yalp

#endif
