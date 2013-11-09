//=============================================================================
/// Symbol manager.
//=============================================================================

#include "symbol_manager.hh"
#include "macp.hh"
#include <assert.h>
#include <string.h>

namespace macp {

Ssymbol* symbol(Svalue s) {
  assert(s.getType() == TT_SYMBOL);
  return static_cast<Ssymbol*>(s.toObject());
};

SymbolManager::SymbolManager()
  : table_() {
}

SymbolManager::~SymbolManager() {
  for (auto key : table_)
    delete[] symbol(key)->c_str();
}

Svalue SymbolManager::intern(const char* name) {
  for (auto v : table_)
    if (strcmp(symbol(v)->c_str(), name) == 0)
      return v;

  const char* copied = copyString(name);
  table_.push_back(Svalue(new Ssymbol(copied)));
  return table_[table_.size() - 1];
}

const char* SymbolManager::copyString(const char* name) {
  int len = strlen(name);
  char* copied = new char[len + 1];
  memcpy(copied, name, len);
  copied[len] = '\0';
  return copied;
}

}  // namespace macp
