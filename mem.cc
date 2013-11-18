#include "mem.hh"
#include <stdlib.h>

namespace yalp {

class DefaultAllocator : public Allocator {
public:
  virtual void* alloc(size_t size) override {
    return ::malloc(size);
  }
  virtual void* realloc(void* p, size_t size) override {
    return ::realloc(p, size);
  }
  virtual void free(void* p) override {
    ::free(p);
  }
};

Allocator* Allocator::getDefaultAllocator() {
  static DefaultAllocator defaultAllocator;
  return &defaultAllocator;
}

}  // namespace yalp
