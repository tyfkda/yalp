#ifndef _ALLOCATOR_HH_
#define _ALLOCATOR_HH_

#include "yalp/gc_object.hh"
#include <stddef.h>  // for size_t

namespace yalp {

typedef void* (*AllocFunc)(void* p, size_t size);

class Allocator {
public:
  struct Callback {
    virtual void allocFailed(void* p, size_t size, void* userdata) = 0;
    virtual void markRoot(void* userdata) = 0;
  };

  static Allocator* create(AllocFunc allocFunc, Callback* callback);
  void release();

  void setUserData(void* userdata)  { userdata_ = userdata; }

  // Allocates non managed memory.
  void* alloc(size_t size);
  void* realloc(void* p, size_t size);
  void free(void* p);

  // Allocates managed memory.
  void* objAlloc(size_t size);

  // Runs garbage collection.
  void collectGarbage();

private:
  Allocator(AllocFunc allocFunc, Callback* callback);
  ~Allocator();

  void sweep();

  AllocFunc allocFunc_;
  Callback* callback_;
  void* userdata_;

  GcObject* objectTop_;
  int objectCount_;
};

AllocFunc getDefaultAllocFunc();

#define ALLOC(allocator, size)  ((allocator)->alloc((size)))
#define REALLOC(allocator, ptr, size)  ((allocator)->realloc((ptr), (size)))
#define FREE(allocator, ptr)  ((allocator)->free((ptr)))
#define OBJALLOC(allocator, size)  ((allocator)->objAlloc((size)))

}  // namespace yalp

#endif
