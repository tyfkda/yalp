//=============================================================================
/// Object
/**

Derived classes should hide their destructor,
and thier destructor should do nothing,
because yalp uses GC and destructor is not called.

Destructor is called if a instance is created explicitly (in C stack).
`destruct` virtual method is called when a instance is managed with
Allocator.
 */
//=============================================================================

#ifndef _OBJECT_HH_
#define _OBJECT_HH_

#include "yalp.hh"
#include "yalp/gc_object.hh"
#include <assert.h>

namespace yalp {

template <class Key>
struct HashPolicy;
template <class Key, class Value>
class HashTable;

class CallStack;
class Stream;

// Base class.
class Object : public GcObject {
public:
  virtual Type getType() const = 0;
  virtual bool equal(const Object* target) const;
  virtual unsigned int calcHash(State* state) const;

  virtual void output(State* state, Stream* o, bool inspect) const = 0;

  virtual bool isCallable() const;

protected:
  // Prevent to call destructor from outside.
  Object()  {}  // Empty construct needed, otherwise member cleared.
  ~Object()  {}

  friend class State;
  friend class Value;
};

// Cell class.
class Cell : public Object {
public:
  Cell(Value a, Value d);
  virtual Type getType() const override;
  virtual bool equal(const Object* target) const override;
  virtual unsigned int calcHash(State* state) const override;

  Value car() const  { return car_; }
  Value cdr() const  { return cdr_; }
  void setCar(Value a);
  void setCdr(Value d);

  virtual void output(State* state, Stream* o, bool inspect) const override;

protected:
  ~Cell()  {}
  virtual void mark() override;

private:
  const char* isAbbrev(State* state) const;

  Value car_;
  Value cdr_;

  friend class State;
};

// String class.
class String : public Object {
public:
  // The given string is allocated in heap and be taken ownership.
  String(const char* string, size_t len);
  virtual Type getType() const override;
  virtual bool equal(const Object* target) const override;
  virtual unsigned int calcHash(State* state) const override;

  const char* c_str() const  { return string_; }
  size_t len() const  { return len_; }

  virtual void output(State* state, Stream* o, bool inspect) const override;

protected:
  ~String()  {}
private:
  virtual void destruct(Allocator* allocator) override;

  const char* string_;
  size_t len_;

  friend class State;
};

#ifndef DISABLE_FLONUM
// Floating point number class.
class SFlonum : public Object {
public:
  SFlonum(Flonum v);
  virtual Type getType() const override;
  virtual bool equal(const Object* target) const override;

  Flonum toFlonum() const  { return v_; }

  virtual void output(State* state, Stream* o, bool inspect) const override;

protected:
  ~SFlonum()  {}

  Flonum v_;

  friend class State;
};
#endif

// Vector class.
class Vector : public Object {
public:
  Vector(Allocator* allocator, int size);
  virtual Type getType() const override;
  virtual bool equal(const Object* target) const override;

  int size() const  { return size_; }

  Value get(int index);
  void set(int index, Value x);

  virtual void output(State* state, Stream* o, bool inspect) const override;

protected:
  ~Vector()  {}
  virtual void destruct(Allocator* allocator) override;
  virtual void mark() override;

  Value* buffer_;
  int size_;

  friend class State;
  friend class Vm;
};

// HashTable class.
class SHashTable : public Object {
public:
  typedef HashTable<Value, Value> TableType;

  explicit SHashTable(Allocator* allocator, HashPolicy<Value>* policy);
  virtual Type getType() const override;

  virtual void output(State* state, Stream* o, bool inspect) const override;

  void put(Value key, Value value);
  const Value* get(Value key) const;
  bool remove(Value key);

  int getCapacity() const;
  int getEntryCount() const;
  int getConflictCount() const;
  int getMaxDepth() const;

  const TableType* getHashTable() const  { return table_; }

protected:
  ~SHashTable();
  virtual void mark() override;

private:
  virtual void destruct(Allocator* allocator) override;

  TableType* table_;

  friend class State;
  friend class Vm;
};

class Callable : public Object {
public:
  Callable();

  virtual bool isCallable() const;

  const Symbol* getName() const  { return name_; }
  void setName(const Symbol* name);

protected:
  const Symbol* name_;
};

// Closure class.
class Closure : public Callable {
public:
  Closure(State* state, Value body, int freeVarCount,
          int minArgNum, int maxArgNum);
  virtual Type getType() const override;

  Value getBody() const  { return body_; }
  int getMinArgNum() const  { return minArgNum_; }
  int getMaxArgNum() const  { return maxArgNum_; }
  bool hasRestParam() const  { return maxArgNum_ < 0; }

  void setFreeVariable(int index, Value value) {
    freeVariables_[index] = value;
  }

  Value getFreeVariable(int index) const {
    assert(0 <= index && index < freeVarCount_);
    return freeVariables_[index];
  }

  virtual void output(State*, Stream* o, bool) const override;

protected:
  ~Closure()  {}
  virtual void destruct(Allocator* allocator) override;
  virtual void mark() override;

  Value body_;
  Value* freeVariables_;
  int freeVarCount_;
  int minArgNum_;
  int maxArgNum_;
};

// Macro class.
class Macro : public Closure {
public:
  Macro(State* state, Value name, Value body, int freeVarCount,
        int minArgNum, int maxArgNum);
  virtual Type getType() const override  { return TT_MACRO; }

  virtual void output(State*, Stream* o, bool) const override;

protected:
  ~Macro()  {}
};

// Native function class.
class NativeFunc : public Callable {
public:
  NativeFunc(NativeFuncType func, int minArgNum, int maxArgNum);
  virtual Type getType() const override;

  int getMinArgNum() const  { return minArgNum_; }
  int getMaxArgNum() const  { return maxArgNum_; }
  Value call(State* state)  { return func_(state); }

  virtual void output(State*, Stream* o, bool) const override;

protected:
  ~NativeFunc()  {}

  NativeFuncType func_;
  int minArgNum_;
  int maxArgNum_;
};

// Continuation class.
class Continuation : public Callable {
public:
  Continuation(State* state, const Value* stack, int size,
               const CallStack* callStack, int callStackSize);
  virtual Type getType() const override;

  int getStackSize() const  { return stackSize_; }
  const Value* getStack() const  { return copiedStack_; }
  int getCallStackSize() const  { return callStackSize_; }
  const CallStack* getCallStack() const  { return callStack_; }

  virtual void output(State*, Stream* o, bool) const override;

protected:
  ~Continuation()  {}
  virtual void destruct(Allocator* allocator) override;
  virtual void mark() override;

  Value* copiedStack_;
  int stackSize_;
  CallStack* callStack_;
  int callStackSize_;
};

class SStream : public Object {
public:
  explicit SStream(Stream* stream);
  virtual Type getType() const override;

  virtual void output(State*, Stream* o, bool) const override;

  inline Stream* getStream() const  { return stream_; }

protected:
  ~SStream()  {}
  virtual void destruct(Allocator* allocator) override;

  Stream* stream_;

  friend class Reader;
  friend class State;
};

}  // namespace yalp

#endif
