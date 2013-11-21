#ifndef _MEM_HH_
#define _MEM_HH_

#include <stdio.h>  // for size_t

namespace yalp {

class Allocator {
public:
  virtual void* alloc(size_t size) = 0;
  virtual void* realloc(void* p, size_t size) = 0;
  virtual void free(void* p) = 0;

  static Allocator* getDefaultAllocator();
};

}  // namespace yalp

#endif
