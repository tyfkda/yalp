//=============================================================================
/// Object
/**

Derived classes should hide their destructor,
and thier destructor should do nothing,
because yalp uses GC and destructor is not called.

 */
//=============================================================================

#ifndef _OBJECT_HH_
#define _OBJECT_HH_

#include "yalp.hh"

#include <ostream>

namespace yalp {

template <class Key, class Value, class Policy>
class HashTable;

// Base class.
class Sobject {
public:
  virtual Type getType() const = 0;
  virtual bool equal(const Sobject* target) const;

  virtual void output(State* state, std::ostream& o, bool inspect) const = 0;

protected:
  // Prevent to call destructor from outside.
  ~Sobject()  {}
};

// Symbol class.
class Symbol : public Sobject {
public:
  virtual Type getType() const override;

  const char* c_str() const  { return name_; }

  virtual void output(State* state, std::ostream& o, bool inspect) const override;

protected:
  Symbol(const char* name);
  ~Symbol()  {}
private:
  const char* name_;

  friend class SymbolManager;
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

  virtual void output(State* state, std::ostream& o, bool inspect) const override;

protected:
  Cell(Svalue a, Svalue d);
  ~Cell()  {}
private:
  Svalue car_;
  Svalue cdr_;

  friend State;
};

// String class.
class String : public Sobject {
public:
  virtual Type getType() const override;
  virtual bool equal(const Sobject* target) const override;

  virtual void output(State* state, std::ostream& o, bool inspect) const override;

protected:
  // The given string is allocated in heap and be taken ownership.
  String(const char* string);
  ~String()  {}
private:
  const char* string_;

  friend State;
};

// HashTable class.
class SHashTable : public Sobject {
public:
  virtual Type getType() const override;

  virtual void output(State* state, std::ostream& o, bool inspect) const override;

  void put(Svalue key, Svalue value);
  const Svalue* get(Svalue key) const;
  bool remove(Svalue key);

  int getCapacity() const;
  int getEntryCount() const;
  int getConflictCount() const;
  int getMaxDepth() const;

protected:
  explicit SHashTable(Allocator* allocator);
  ~SHashTable()  {}
private:
  struct Policy {
    static unsigned int hash(const Svalue a)  { return a.calcHash(); }
    static bool equal(const Svalue a, const Svalue b)  { return a.eq(b); }
  };

  HashTable<Svalue, Svalue, Policy>* table_;

  friend State;
  friend Vm;
};

}  // namespace yalp

#endif
