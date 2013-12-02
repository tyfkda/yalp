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
  typedef HashTable<const char*, Symbol*> TableType;
  struct StrHashPolicy;

  static SymbolManager* create(Allocator* allocator);
  // Delete.
  void release();

  // Create symbol from c-string.
  Symbol* intern(const char* name);

  // Generate new symbol.
  Symbol* gensym();

  void reportDebugInfo() const;

private:
  SymbolManager(Allocator* allocator);
  ~SymbolManager();
  Symbol* generate(const char* name);
  char* copyString(const char* name);

  static StrHashPolicy s_hashPolicy;

  Allocator* allocator_;
  TableType table_;
  int gensymIndex_;
};

}  // namespace yalp

#endif
