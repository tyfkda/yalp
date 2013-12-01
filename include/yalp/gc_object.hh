#ifndef _GC_OBJECT_HH_
#define _GC_OBJECT_HH_

namespace yalp {

class Allocator;

class GcObject {
protected:
  GcObject()  {}  // Empty construct needed, otherwise member cleared.
  ~GcObject()  {}
  virtual void destruct(Allocator*)  {}

  virtual void mark()  { marked_ = true; }
  bool isMarked() const  { return marked_; }

private:
  GcObject* next_;
  bool marked_;

  friend Allocator;
};

}  // namespace yalp

#endif
