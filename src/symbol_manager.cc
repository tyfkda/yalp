//=============================================================================
/// Symbol manager.
//=============================================================================

#include "symbol_manager.hh"
#include "yalp/object.hh"
#include <assert.h>
#include <iostream>
#include <string.h>

namespace yalp {

struct SymbolManager::StrHashPolicy : public HashPolicy<const char*> {
  virtual unsigned int hash(const char* a) override  { return strHash(a); }
  virtual bool equal(const char* a, const char* b) override  { return strcmp(a, b) == 0; }
};

SymbolManager::StrHashPolicy SymbolManager::s_hashPolicy;

SymbolManager* SymbolManager::create(Allocator* allocator) {
  void* memory = ALLOC(allocator, sizeof(SymbolManager));
  return new(memory) SymbolManager(allocator);
}

void SymbolManager::release() {
  Allocator* allocator = allocator_;
  this->~SymbolManager();
  FREE(allocator, this);
}

SymbolManager::SymbolManager(Allocator* allocator)
  : allocator_(allocator)
  , table_(&s_hashPolicy, allocator)
  , gensymIndex_(0) {
}

SymbolManager::~SymbolManager() {
  for (auto symbol : array_) {
    FREE(allocator_, symbol.name_);
    //FREE(allocator_, symbol);
  }
}

SymbolId SymbolManager::intern(const char* name) {
  const SymbolId* result = table_.get(name);
  if (result != NULL)
    return *result;
  return generate(name);
}

SymbolId SymbolManager::gensym() {
  int no = ++gensymIndex_;
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "#G:%d", no);
  return generate(buffer);
}

const Symbol* SymbolManager::get(SymbolId symbolId) const {
  return &array_[symbolId];
}

SymbolId SymbolManager::generate(const char* name) {
  char* copied = copyString(name);
  SymbolId symbolId = array_.size();
  array_.push_back(Symbol(copied));
  Symbol* symbol = &array_[symbolId];
  table_.put(symbol->c_str(), symbolId);
  return symbolId;
}

char* SymbolManager::copyString(const char* name) {
  int len = strlen(name);
  char* copied = static_cast<char*>(ALLOC(allocator_, len + 1));
  memcpy(copied, name, len);
  copied[len] = '\0';
  return copied;
}

void SymbolManager::reportDebugInfo() const {
  std::cout << "Symbols:" << std::endl;
  std::cout << "  capacity: #" << table_.getCapacity() << std::endl;
  std::cout << "  entry:    #" << table_.getEntryCount() << std::endl;
  std::cout << "  conflict: #" << table_.getConflictCount() << std::endl;
  std::cout << "  maxdepth: #" << table_.getMaxDepth() << std::endl;
}

}  // namespace yalp
