//=============================================================================
/// Symbol manager.
//=============================================================================

#ifndef _SYMBOL_MANAGER_HH_
#define _SYMBOL_MANAGER_HH_

#include "hash_table.hh"

namespace yalp {

class Allocator;

typedef int SymbolId;

// Symbol class
class Symbol {
public:
  explicit Symbol(char* name);
  ~Symbol()  {}

  unsigned int getHash() const  { return hash_; }
  const char* c_str() const  { return name_; }

private:
  char* name_;
  unsigned int hash_;  // Pre-calculated hash value.
};

class SymbolManager {
public:
  typedef HashTable<const char*, SymbolId> TableType;
  struct StrHashPolicy;

  static SymbolManager* create(Allocator* allocator);
  // Delete.
  void release();

  // Create symbol from c-string.
  SymbolId intern(const char* name);

  const Symbol* get(SymbolId symbolId) const;

  void reportDebugInfo() const;

private:
  SymbolManager(Allocator* allocator);
  ~SymbolManager();
  SymbolId generate(const char* name);
  void expandSymbolPage(SymbolId oldSize);
  void expandNamePage(size_t len);
  char* copyString(const char* name);

  static StrHashPolicy s_hashPolicy;

  Allocator* allocator_;
  TableType table_;

  // Memory blocks for Symbol instances.
  struct SymbolPage;
  SymbolPage* symbolPageTop_;
  Symbol** symbolArray_;
  SymbolId symbolIndex_;

  // Memory blocks for Names.
  struct NamePage;
  NamePage* namePageTop_;
  size_t nameBufferSize_;
  size_t nameBufferOffset_;
};

}  // namespace yalp

#endif
