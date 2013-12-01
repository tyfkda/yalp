#ifndef _GC_OBJECT_HH_
#define _GC_OBJECT_HH_

namespace yalp {

class Allocator;

class GcObject {
protected:
  GcObject()  {}  // Empty construct needed, otherwise member cleared.
  ~GcObject()  {}
  virtual void destruct(Allocator*)  {}

private:
  GcObject* next_;
  bool marked_;

  friend Allocator;
};

}  // namespace yalp

#endif
