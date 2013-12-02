//=============================================================================
/// Symbol manager.
//=============================================================================

#ifndef _SYMBOL_MANAGER_HH_
#define _SYMBOL_MANAGER_HH_

#include "hash_table.hh"
#include <vector>

namespace yalp {

class Allocator;
class Symbol;

typedef unsigned int SymbolId;

class SymbolManager {
public:
  typedef HashTable<const char*, SymbolId> TableType;
  struct StrHashPolicy;

  static SymbolManager* create(Allocator* allocator);
  // Delete.
  void release();

  // Create symbol from c-string.
  SymbolId intern(const char* name);

  // Generate new symbol.
  SymbolId gensym();

  const Symbol* get(SymbolId symbolId) const;

  void reportDebugInfo() const;

private:
  SymbolManager(Allocator* allocator);
  ~SymbolManager();
  SymbolId generate(const char* name);
  char* copyString(const char* name);

  static StrHashPolicy s_hashPolicy;

  Allocator* allocator_;
  TableType table_;
  int gensymIndex_;

  std::vector<Symbol> array_;
};

}  // namespace yalp

#endif
