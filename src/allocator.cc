#include "allocator.hh"
#include <stdlib.h>  // for malloc, free

namespace yalp {

void* defaultAllocFunc(void* p, size_t size) {
  if (size <= 0) {
    free(p);
    return NULL;
  }
  if (p == NULL)
    return malloc(size);
  return realloc(p, size);
}

AllocFunc getDefaultAllocFunc()  { return defaultAllocFunc; }

Allocator::Allocator(State* state, AllocFunc allocFunc)
  : state_(state), allocFunc_(allocFunc) {
}

void* Allocator::alloc(size_t size) {
  return allocFunc_(NULL, size);
}

void* Allocator::realloc(void* p, size_t size) {
  return allocFunc_(p, size);
}

void Allocator::free(void* p) {
  allocFunc_(p, 0);
}

}  // namespace yalp
