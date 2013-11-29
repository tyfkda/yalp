#ifndef _ALLOCATOR_HH_
#define _ALLOCATOR_HH_

#include <stdio.h>  // for size_t

namespace yalp {

class State;

typedef void* (*AllocFunc)(void*p, size_t size);

class Allocator {
public:
  Allocator(State* state, AllocFunc allocFunc);

  void* alloc(size_t size);
  void* realloc(void* p, size_t size);
  void free(void* p);

private:
  State* state_;
  AllocFunc allocFunc_;

  friend State;
};

AllocFunc getDefaultAllocFunc();

}  // namespace yalp

#endif
