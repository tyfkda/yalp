//=============================================================================
/// Symbol manager.
//=============================================================================

#include "build_env.hh"
#include "symbol_manager.hh"

#include <assert.h>
#include <iostream>
#include <string.h>

namespace yalp {

// Number of objects which a page contains.
const int PAGE_OBJECT_COUNT = 64;

// Number of objects which a page contains.
const int NAME_BUFFER_SIZE = 1024 - sizeof(void*);

//=============================================================================

static unsigned int strHash(const char* s) {
  unsigned int v = 0;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
       *p != '\0'; ++p)
    v = v * 17 + 1 + *p;
  return v;
}

Symbol::Symbol(char* name)
  : name_(name), hash_(strHash(name)) {}

//=============================================================================
/*
  A memory page is a fixed sized memory block,
  and it contains `PAGE_OBJECT_COUNT` Symbols.
 */
struct Page {
  Page* next;

  explicit Page(Page* p) : next(p)  {}
};

struct SymbolManager::SymbolPage : public Page {
  char symbolBuffer[PAGE_OBJECT_COUNT][sizeof(Symbol)];  // Watch out alignment.

  explicit SymbolPage(SymbolPage* p) : Page(p)  {}
};

struct SymbolManager::NamePage : public Page {
  char buffer[1];  // Buffer size is expanded.

  explicit NamePage(NamePage* p) : Page(p)  {}
};

//=============================================================================

struct SymbolManager::StrHashPolicy : public HashPolicy<const char*> {
  virtual unsigned int hash(const char* a) override  { return strHash(a); }
  virtual bool equal(const char* a, const char* b) override  { return strcmp(a, b) == 0; }
};

SymbolManager::StrHashPolicy SymbolManager::s_hashPolicy;

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
  , table_(&s_hashPolicy, allocator)
  , symbolPageTop_(NULL), symbolArray_(NULL), symbolIndex_(0)
  , namePageTop_(NULL), nameBufferSize_(0), nameBufferOffset_(0) {
}

SymbolManager::~SymbolManager() {
  // Frees name pages.
  for (Page* page = namePageTop_; page != NULL; ) {
    Page* next = page->next;
    allocator_->free(page);
    page = next;
  }
  // Frees symbol pages.
  for (Page* page = symbolPageTop_; page != NULL; ) {
    Page* next = page->next;
    allocator_->free(page);
    page = next;
  }
  // Frees array.
  if (symbolArray_ != NULL)
    allocator_->free(symbolArray_);
}

SymbolId SymbolManager::intern(const char* name) {
  const SymbolId* result = table_.get(name);
  if (result != NULL)
    return *result;
  SymbolId symbolId = generate(name);
  table_.put(get(symbolId)->c_str(), symbolId);
  return symbolId;
}

const Symbol* SymbolManager::get(SymbolId symbolId) const {
  assert(0 <= symbolId && symbolId < symbolIndex_);
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
  SymbolPage* newPage = new(allocator_->alloc(sizeof(*newPage))) SymbolPage(symbolPageTop_);
  symbolPageTop_ = newPage;

  symbolArray_ = static_cast<Symbol**>(allocator_->realloc(symbolArray_,
                                                           sizeof(Symbol*) * extendedCount));
}

void SymbolManager::expandNamePage(size_t len) {
  size_t bufferSize = NAME_BUFFER_SIZE;
  if (bufferSize <= len) {
    bufferSize = (len / NAME_BUFFER_SIZE + 1) * NAME_BUFFER_SIZE;
  }

  NamePage* newPage = new(allocator_->alloc(sizeof(*newPage) + bufferSize - 1)) NamePage(namePageTop_);
  namePageTop_ = newPage;
  nameBufferSize_ = bufferSize;
  nameBufferOffset_ = 0;
}

char* SymbolManager::copyString(const char* name) {
  size_t len = strlen(name);
  if (namePageTop_ == NULL || nameBufferSize_ - nameBufferOffset_ <= len)
    expandNamePage(len);
  char* copied = &namePageTop_->buffer[nameBufferOffset_];
  nameBufferOffset_ += len + 1;
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
