//=============================================================================
// GcObject
/*
 * Base class for memory managed object.
 * `destruct` method is called instead of destructor.
 */
//=============================================================================

#ifndef _GC_OBJECT_HH_
#define _GC_OBJECT_HH_

namespace yalp {

class Allocator;

class GcObject {
protected:
  GcObject()  {}  // Empty construct needed, otherwise member cleared.
  ~GcObject()  {}  // Destructor is not called, so no need virtual modifier.
  virtual void destruct(Allocator*)  {}

  virtual void mark();
  bool isMarked() const;

private:
  void clearMark();

  GcObject* next_;

  friend class Allocator;
};

}  // namespace yalp

#endif
