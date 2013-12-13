//=============================================================================
/// Symbol manager.
//=============================================================================

#include "symbol_manager.hh"
#include "yalp/object.hh"
#include <assert.h>
#include <iostream>
#include <string.h>

namespace yalp {

// Number of objects which a page contains.
const int PAGE_OBJECT_COUNT = 64;

//=============================================================================
/*
  A memory page is a fixed sized memory block,
  and it contains `PAGE_OBJECT_COUNT` Symbols.
 */
struct SymbolManager::Page {
  Page* next;
  char symbolBuffer[PAGE_OBJECT_COUNT][sizeof(Symbol)];  // Watch out alignment.

  explicit Page(Page* p) : next(p)  {}
};

//=============================================================================

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
  , gensymIndex_(0)
  , symbolPageTop_(NULL), symbolArray_(NULL), symbolIndex_(0) {
}

SymbolManager::~SymbolManager() {
  // Frees symbols.
  for (SymbolId i = 0; i < symbolIndex_; ++i) {
    Symbol* symbol = symbolArray_[i];
    FREE(allocator_, symbol->name_);
  }
  // Frees pages.
  for (Page* page = symbolPageTop_; page != NULL; ) {
    Page* next = page->next;
    FREE(allocator_, page);
    page = next;
  }
  // Frees array.
  FREE(allocator_, symbolArray_);
}

SymbolId SymbolManager::intern(const char* name) {
  const SymbolId* result = table_.get(name);
  if (result != NULL)
    return *result;
  SymbolId symbolId = generate(name);
  table_.put(get(symbolId)->c_str(), symbolId);
  return symbolId;
}

SymbolId SymbolManager::gensym() {
  int no = ++gensymIndex_;
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "#G:%d", no);
  return generate(buffer);
}

const Symbol* SymbolManager::get(SymbolId symbolId) const {
  return symbolArray_[symbolId];
}

SymbolId SymbolManager::generate(const char* name) {
  char* copied = copyString(name);
  SymbolId symbolId = symbolIndex_++;

  int offset = symbolId & (PAGE_OBJECT_COUNT - 1);
  if (offset == 0)
    expandSymbolPage(symbolId);

  Symbol* symbol = new(symbolPageTop_->symbolBuffer[offset]) Symbol(copied);
  symbolArray_[symbolId] = symbol;
  return symbolId;
}

void SymbolManager::expandSymbolPage(SymbolId oldSize) {
  SymbolId extendedCount = oldSize + PAGE_OBJECT_COUNT;
  Page* newPage = new(ALLOC(allocator_, sizeof(*newPage))) Page(symbolPageTop_);
  symbolPageTop_ = newPage;

  symbolArray_ = static_cast<Symbol**>(REALLOC(allocator_, symbolArray_,
                                         sizeof(Symbol*) * extendedCount));
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
