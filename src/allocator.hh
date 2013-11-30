#ifndef _ALLOCATOR_HH_
#define _ALLOCATOR_HH_

#include <stdio.h>  // for size_t

namespace yalp {

class State;

typedef void* (*AllocFunc)(void*p, size_t size);

class Allocator {
public:
  struct Callback {
    virtual void destruct(void* memory, State* state) = 0;
  };

  static Allocator* create(State* state, AllocFunc allocFunc, Callback* callback);
  void release();

  // Non managed memory allocation.
  void* alloc(size_t size);
  void* realloc(void* p, size_t size);
  void free(void* p);

  // Managed memory allocation.
  void* objAlloc(size_t size);

private:
  Allocator(State* state, AllocFunc allocFunc, Callback* callback);
  ~Allocator();

  State* state_;
  AllocFunc allocFunc_;
  Callback* callback_;

  struct Link {
    Link* next;
    void* memory;
  };
  Link* objectTop_;

  friend State;
};

AllocFunc getDefaultAllocFunc();

#define RAW_ALLOC(allocFunc, size)  (allocFunc(NULL, size))
#define RAW_FREE(allocFunc, ptr)  (allocFunc(ptr, 0))

#define ALLOC(allocator, size)  ((allocator)->alloc((size)))
#define REALLOC(allocator, ptr, size)  ((allocator)->realloc((ptr), (size)))
#define FREE(allocator, ptr)  ((allocator)->free((ptr)))
#define OBJALLOC(allocator, size)  ((allocator)->objAlloc((size)))

}  // namespace yalp

#endif
