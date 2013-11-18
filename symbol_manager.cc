//=============================================================================
/// Symbol manager.
//=============================================================================

#include "symbol_manager.hh"
#include "yalp.hh"
#include <assert.h>
#include <string.h>

namespace yalp {

SymbolManager::SymbolManager()
  : table_() {
}

SymbolManager::~SymbolManager() {
  for (auto symbol : table_)
    delete[] symbol->c_str();
}

Symbol* SymbolManager::intern(const char* name) {
  for (auto symbol : table_)
    if (strcmp(symbol->c_str(), name) == 0)
      return symbol;

  const char* copied = copyString(name);
  Symbol* symbol = new Symbol(copied);
  table_.push_back(symbol);
  return symbol;
}

const char* SymbolManager::copyString(const char* name) {
  int len = strlen(name);
  char* copied = new char[len + 1];
  memcpy(copied, name, len);
  copied[len] = '\0';
  return copied;
}

}  // namespace yalp
