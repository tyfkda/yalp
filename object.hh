//=============================================================================
/// object
//=============================================================================

#ifndef _VALUE_HH_
#define _VALUE_HH_

#include "yalp.hh"

#include <ostream>

namespace yalp {

// Base class.
class Sobject {
public:
  virtual ~Sobject();
  virtual Type getType() const = 0;
  virtual bool equal(const Sobject* target) const;

  virtual std::ostream& operator<<(std::ostream& o) const = 0;
};

inline std::ostream& operator<<(std::ostream& o, Sobject& object) {
  return object.operator<<(o);
}

// Symbol class.
class Symbol : public Sobject {
public:
  virtual Type getType() const override;

  const char* c_str() const  { return name_; }

  virtual std::ostream& operator<<(std::ostream& o) const override;

protected:
  Symbol(const char* name);
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
  void rplaca(Svalue a);
  void rplacd(Svalue d);

  virtual std::ostream& operator<<(std::ostream& o) const override;

protected:
  Cell(Svalue a, Svalue d);
  Svalue car_;
  Svalue cdr_;

  friend State;
};

// String class.
class String : public Sobject {
public:
  virtual Type getType() const override;
  virtual bool equal(const Sobject* target) const override;

  virtual std::ostream& operator<<(std::ostream& o) const override;

protected:
  String(const char* string);
  const char* string_;

  friend State;
};

}  // namespace yalp

#endif
