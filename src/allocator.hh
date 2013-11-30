#ifndef _ALLOCATOR_HH_
#define _ALLOCATOR_HH_

#include <stdio.h>  // for size_t

namespace yalp {

class State;

typedef void* (*AllocFunc)(void*p, size_t size);

class Allocator {
public:
  static Allocator* create(State* state, AllocFunc allocFunc);
  void release();

  // Non managed memory allocation.
  void* alloc(size_t size);
  void* realloc(void* p, size_t size);
  void free(void* p);

  // Managed memory allocation.
  void* objAlloc(size_t size);

private:
  Allocator(State* state, AllocFunc allocFunc);
  ~Allocator();

  State* state_;
  AllocFunc allocFunc_;

  struct Link {
    Link* next;
    void* memory;
  };
  Link* objectTop_;

  friend State;
};

AllocFunc getDefaultAllocFunc();

}  // namespace yalp

#endif
