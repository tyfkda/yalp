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
  , objectTop_(NULL), objectCount_(0) {
  (void)callback_;
  (void)userdata_;
}

Allocator::~Allocator() {
  while (objectTop_ != NULL) {
    GcObject* gcobj = objectTop_;
    objectTop_ = gcobj->next_;
    gcobj->destruct(this);
    FREE(this, gcobj);
  }
}

void* Allocator::alloc(size_t size) {
  void* q = RAW_ALLOC(allocFunc_, size);
  if (q == NULL) {
    collectGarbage();
    q = RAW_ALLOC(allocFunc_, size);
    if (q == NULL)
      callback_->allocFailed(NULL, size, userdata_);
  }
  return q;
}

void* Allocator::realloc(void* p, size_t size) {
  void* q = RAW_REALLOC(allocFunc_, p, size);
  if (q == NULL) {
    collectGarbage();
    q = RAW_REALLOC(allocFunc_, p, size);
    if (q == NULL)
      callback_->allocFailed(NULL, size, userdata_);
  }
  return q;
}

void Allocator::free(void* p) {
  RAW_FREE(allocFunc_, p);
}

void* Allocator::objAlloc(size_t size) {
  GcObject* gcobj = static_cast<GcObject*>(ALLOC(this, size));
  gcobj->next_ = objectTop_;
  gcobj->marked_ = false;
  objectTop_ = gcobj;
  ++objectCount_;
  return gcobj;
}

void Allocator::collectGarbage() {
  callback_->markRoot(userdata_);
  sweep();
}

void Allocator::sweep() {
  int n = 0;
  GcObject* prev = NULL;
  for (GcObject* gcobj = objectTop_; gcobj != NULL; ) {
    if (gcobj->marked_) {
      prev = gcobj;
      gcobj->marked_ = false;
      gcobj = gcobj->next_;
      ++n;
      continue;
    }

    GcObject* next = gcobj->next_;
    if (prev == NULL)
      objectTop_ = next;
    else
      prev->next_ = next;
    gcobj->destruct(this);
    FREE(this, gcobj);
    gcobj = next;
  }
  objectCount_ = n;
}

}  // namespace yalp
