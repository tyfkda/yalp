#include "allocator.hh"
#include <new>
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

Allocator* Allocator::create(AllocFunc allocFunc, Callback* callback, void* userdata) {
  void* memory = RAW_ALLOC(allocFunc, sizeof(Allocator));
  return new(memory) Allocator(allocFunc, callback, userdata);
}

void Allocator::release() {
  AllocFunc allocFunc = allocFunc_;
  this->~Allocator();
  RAW_FREE(allocFunc, this);
}

Allocator::Allocator(AllocFunc allocFunc, Callback* callback, void* userdata)
  : allocFunc_(allocFunc), callback_(callback), userdata_(userdata)
  , objectTop_(NULL) {
}

Allocator::~Allocator() {
  while (objectTop_ != NULL) {
    Link* link = objectTop_;
    objectTop_ = link->next;
    callback_->destruct(link->memory, userdata_);
    FREE(this, link->memory);
    FREE(this, link);
  }
}

void* Allocator::alloc(size_t size) {
  return RAW_ALLOC(allocFunc_, size);
}

void* Allocator::realloc(void* p, size_t size) {
  return RAW_REALLOC(allocFunc_, p, size);
}

void Allocator::free(void* p) {
  RAW_FREE(allocFunc_, p);
}

void* Allocator::objAlloc(size_t size) {
  void* memory = ALLOC(this, size);
  Link* link = static_cast<Link*>(ALLOC(this, sizeof(Link)));
  link->next = objectTop_;
  link->memory = memory;
  objectTop_ = link;
  return memory;
}

}  // namespace yalp
