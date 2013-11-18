//=============================================================================
/// Symbol manager.
//=============================================================================

#include "symbol_manager.hh"
#include "object.hh"
#include <assert.h>
#include <string.h>

namespace yalp {

SymbolManager* SymbolManager::create(Allocator* allocator) {
  void* memory = allocator->alloc(sizeof(SymbolManager));
  return new(memory) SymbolManager(allocator);
}

void SymbolManager::release() {
  Allocator* allocator = allocator_;
  this->~SymbolManager();
  allocator->free(this);
}

SymbolManager::SymbolManager(Allocator* allocator)
  : allocator_(allocator)
  , table_() {
  table_.reserve(100);
}

SymbolManager::~SymbolManager() {
  for (auto symbol : table_) {
    allocator_->free(const_cast<char*>(symbol->c_str()));
    allocator_->free(symbol);
  }
}

Symbol* SymbolManager::intern(const char* name) {
  for (auto symbol : table_)
    if (strcmp(symbol->c_str(), name) == 0)
      return symbol;

  const char* copied = copyString(name);
  void* memory = allocator_->alloc(sizeof(Symbol));
  Symbol* symbol = new(memory) Symbol(copied);
  table_.push_back(symbol);
  return symbol;
}

const char* SymbolManager::copyString(const char* name) {
  int len = strlen(name);
  char* copied = static_cast<char*>(allocator_->alloc(len + 1));
  memcpy(copied, name, len);
  copied[len] = '\0';
  return copied;
}

}  // namespace yalp
