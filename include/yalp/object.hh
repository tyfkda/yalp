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

namespace yalp {

template <class Key>
struct HashPolicy;
template <class Key, class Value>
class HashTable;

class Stream;

// Symbol class
class Symbol {
public:
  ~Symbol()  {}

  unsigned int getHash() const  { return hash_; }
  const char* c_str() const  { return name_; }

private:
  explicit Symbol(char* name);

  char* name_;
  unsigned int hash_;  // Pre-calculated hash value.

  friend class SymbolManager;
};

// Base class.
class Sobject : public GcObject {
public:
  virtual Type getType() const = 0;
  virtual bool equal(const Sobject* target) const;
  virtual unsigned int calcHash() const;

  virtual void output(State* state, Stream* o, bool inspect) const = 0;

  virtual bool isCallable() const;

protected:
  // Prevent to call destructor from outside.
  Sobject()  {}  // Empty construct needed, otherwise member cleared.
  ~Sobject()  {}

  friend State;
  friend Svalue;
};

// Cell class.
class Cell : public Sobject {
public:
  virtual Type getType() const override;
  virtual bool equal(const Sobject* target) const override;

  Svalue car() const  { return car_; }
  Svalue cdr() const  { return cdr_; }
  void setCar(Svalue a);
  void setCdr(Svalue d);

  virtual void output(State* state, Stream* o, bool inspect) const override;

protected:
  Cell(Svalue a, Svalue d);
  ~Cell()  {}
  virtual void mark();

private:
  const char* isAbbrev(State* state) const;

  Svalue car_;
  Svalue cdr_;

  friend State;
};

// String class.
class String : public Sobject {
public:
  virtual Type getType() const override;
  virtual bool equal(const Sobject* target) const override;
  virtual unsigned int calcHash() const override;

  const char* c_str() const  { return string_; }
  int len() const  { return len_; }

  virtual void output(State* state, Stream* o, bool inspect) const override;

protected:
  // The given string is allocated in heap and be taken ownership.
  String(const char* string, int len);
  ~String()  {}
private:
  virtual void destruct(Allocator* allocator) override;

  const char* string_;
  int len_;

  friend State;
};

// Floating point number class.
class Float : public Sobject {
public:
  virtual Type getType() const override;
  virtual bool equal(const Sobject* target) const override;

  Sfloat toFloat() const  { return v_; }

  virtual void output(State* state, Stream* o, bool inspect) const override;

protected:
  Float(Sfloat v);
  ~Float()  {}

  Sfloat v_;

  friend State;
};

// Vector class.
class Vector : public Sobject {
public:
  virtual Type getType() const override;

  int size() const  { return size_; }

  Svalue get(int index);
  void set(int index, Svalue x);

  virtual void output(State* state, Stream* o, bool inspect) const override;

protected:
  Vector(Allocator* allocator, int size);
  ~Vector()  {}
  virtual void destruct(Allocator* allocator) override;
  virtual void mark();

  Svalue* buffer_;
  int size_;

  friend State;
  friend Vm;
};

// HashTable class.
class SHashTable : public Sobject {
public:
  typedef HashTable<Svalue, Svalue> TableType;

  virtual Type getType() const override;

  virtual void output(State* state, Stream* o, bool inspect) const override;

  void put(Svalue key, Svalue value);
  const Svalue* get(Svalue key) const;
  bool remove(Svalue key);

  int getCapacity() const;
  int getEntryCount() const;
  int getConflictCount() const;
  int getMaxDepth() const;

  const TableType* getHashTable() const  { return table_; }

protected:
  explicit SHashTable(Allocator* allocator, HashPolicy<Svalue>* policy);
  ~SHashTable();
  virtual void mark();

private:
  virtual void destruct(Allocator* allocator) override;

  TableType* table_;

  friend State;
  friend Vm;
};

class Callable : public Sobject {
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
  Closure(State* state, Svalue body, int freeVarCount,
          int minArgNum, int maxArgNum);
  virtual Type getType() const override;

  Svalue getBody() const  { return body_; }
  int getMinArgNum() const  { return minArgNum_; }
  int getMaxArgNum() const  { return maxArgNum_; }
  bool hasRestParam() const  { return maxArgNum_ < 0; }

  void setFreeVariable(int index, Svalue value) {
    freeVariables_[index] = value;
  }

  Svalue getFreeVariable(int index) const {
    return freeVariables_[index];
  }

  virtual void output(State*, Stream* o, bool) const override;

protected:
  ~Closure()  {}
  virtual void destruct(Allocator* allocator) override;
  virtual void mark();

  Svalue body_;
  Svalue* freeVariables_;
  int freeVarCount_;
  int minArgNum_;
  int maxArgNum_;
};

// Native function class.
class NativeFunc : public Callable {
public:
  NativeFunc(NativeFuncType func, int minArgNum, int maxArgNum);
  virtual Type getType() const override;

  Svalue call(State* state, int argNum);

  virtual void output(State*, Stream* o, bool) const override;

protected:
  ~NativeFunc()  {}

  NativeFuncType func_;
  int minArgNum_;
  int maxArgNum_;
};

class SStream : public Sobject {
public:
  virtual Type getType() const override;

  virtual void output(State*, Stream* o, bool) const override;

  inline Stream* getStream() const  { return stream_; }

protected:
  explicit SStream(Stream* stream);
  ~SStream()  {}
  virtual void destruct(Allocator* allocator) override;

  Stream* stream_;

  friend State;
};

}  // namespace yalp

#endif
