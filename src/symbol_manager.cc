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
  for (auto it = table_.begin(); it != table_.end(); ++it) {
    Symbol* symbol = it->value;
    FREE(allocator_, const_cast<char*>(symbol->c_str()));
    FREE(allocator_, symbol);
  }
}

Symbol* SymbolManager::intern(const char* name) {
  Symbol* const* result = table_.get(name);
  if (result != NULL)
    return *result;
  return generate(name);
}

Symbol* SymbolManager::gensym() {
  int no = ++gensymIndex_;
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "#G:%d", no);
  return generate(buffer);
}

Symbol* SymbolManager::generate(const char* name) {
  const char* copied = copyString(name);
  void* memory = ALLOC(allocator_, sizeof(Symbol));
  Symbol* symbol = new(memory) Symbol(copied);
  table_.put(symbol->c_str(), symbol);
  return symbol;
}

const char* SymbolManager::copyString(const char* name) {
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
