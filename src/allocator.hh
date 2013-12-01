#ifndef _ALLOCATOR_HH_
#define _ALLOCATOR_HH_

#include "yalp/gc_object.hh"
#include <stdio.h>  // for size_t

namespace yalp {

typedef void* (*AllocFunc)(void*p, size_t size);

class Allocator {
public:
  struct Callback {
  };

  static Allocator* create(AllocFunc allocFunc, Callback* callback, void* userdata);
  void release();

  // Non managed memory allocation.
  void* alloc(size_t size);
  void* realloc(void* p, size_t size);
  void free(void* p);

  // Managed memory allocation.
  void* objAlloc(size_t size);

private:
  Allocator(AllocFunc allocFunc, Callback* callback, void* userdata);
  ~Allocator();

  AllocFunc allocFunc_;
  Callback* callback_;
  void* userdata_;

  GcObject* objectTop_;
};

AllocFunc getDefaultAllocFunc();

#define RAW_ALLOC(allocFunc, size)  (allocFunc(NULL, (size)))
#define RAW_REALLOC(allocFunc, ptr, size)  (allocFunc((ptr), (size)))
#define RAW_FREE(allocFunc, ptr)  (allocFunc((ptr), 0))

#define ALLOC(allocator, size)  ((allocator)->alloc((size)))
#define REALLOC(allocator, ptr, size)  ((allocator)->realloc((ptr), (size)))
#define FREE(allocator, ptr)  ((allocator)->free((ptr)))
#define OBJALLOC(allocator, size)  ((allocator)->objAlloc((size)))

}  // namespace yalp

#endif
