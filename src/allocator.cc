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

Allocator* Allocator::create(State* state, AllocFunc allocFunc, Callback* callback) {
  void* memory = allocFunc(NULL, sizeof(Allocator));
  return new(memory) Allocator(state, allocFunc, callback);
}

void Allocator::release() {
  AllocFunc allocFunc = allocFunc_;
  this->~Allocator();
  allocFunc(this, 0);
}

Allocator::Allocator(State* state, AllocFunc allocFunc, Callback* callback)
  : state_(state), allocFunc_(allocFunc), callback_(callback)
  , objectTop_(NULL) {
}

Allocator::~Allocator() {
  while (objectTop_ != NULL) {
    Link* link = objectTop_;
    objectTop_ = link->next;
    callback_->destruct(link->memory, state_);
    free(link->memory);
    free(link);
  }
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

void* Allocator::objAlloc(size_t size) {
  void* memory = allocFunc_(NULL, size);
  Link* link = static_cast<Link*>(allocFunc_(NULL, sizeof(Link)));
  link->next = objectTop_;
  link->memory = memory;
  objectTop_ = link;
  return memory;
}

}  // namespace yalp
