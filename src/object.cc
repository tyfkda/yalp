//=============================================================================
/// Object
//=============================================================================

#include "build_env.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"  // for strHash
#include "hash_table.hh"
#include "symbol_manager.hh"
#include "vm.hh"  // for CallStack

#include <assert.h>
#include <new>
#include <string.h>  // for memcmp

namespace yalp {

//=============================================================================
bool Object::equal(const Object* o) const {
  return this == o;  // Simple pointer equality.
}

unsigned int Object::calcHash(State*) const {
  return (reinterpret_cast<long>(this) >> 4) * 23;
}

bool Object::isCallable() const  { return false; }

//=============================================================================
Cell::Cell(Value a, Value d)
  : Object(), car_(a), cdr_(d) {}

Type Cell::getType() const  { return TT_CELL; }

bool Cell::equal(const Object* target) const {
  const Cell* p = static_cast<const Cell*>(target);
  return car_.equal(p->car_) && cdr_.equal(p->cdr_);
}

unsigned int Cell::calcHash(State* state) const {
  const int MUL = 37, ADD = 1;
  unsigned int hash = 0;
  // Need to avoid infinite loop.
  const int N = 8;
  const Cell* queue[N];
  int w = 0, r = 0;
  queue[w++] = this;
  while (r < w) {
    const Cell* p = queue[r++];
    for (auto v : { p->car(), p->cdr() }) {
      if (v.getType() == TT_CELL) {
        if (w < N)
          queue[w++] = static_cast<Cell*>(v.toObject());
        continue;
      }
      // TODO: Handle mutual recursive with other container type (i.e. vector).
      hash = hash * MUL + p->car().calcHash(state) + ADD;
    }
  }
  return hash;
}

void Cell::output(State* state, Stream* o, bool inspect) const {
  if (state != NULL) {
    const char* abbrev = isAbbrev(state);
    if (abbrev != NULL) {
      o->write(abbrev);
      yalp::car(cdr()).output(state, o, inspect);
      return;
    }
  }

  char c = '(';
  const Cell* p;
  for (p = this; ; p = static_cast<Cell*>(p->cdr_.toObject())) {
    o->write(c);
    p->car_.output(state, o, inspect);
    if (p->cdr_.getType() != TT_CELL)
      break;
    c = ' ';
  }
  if (!p->cdr_.eq(Value::NIL)) {
    o->write(" . ");
    p->cdr_.output(state, o, inspect);
  }
  o->write(')');
}

void Cell::mark() {
  if (isMarked())
    return;
  Object::mark();
  car_.mark();
  cdr_.mark();
}

void Cell::setCar(Value a) {
  car_ = a;
}

void Cell::setCdr(Value d) {
  cdr_ = d;
}

const char* Cell::isAbbrev(State* state) const {
  if (car().getType() != TT_SYMBOL ||
      cdr().getType() != TT_CELL ||
      !yalp::cdr(cdr()).eq(Value::NIL))
    return NULL;

  struct {
    State::Constant c;
    const char* abbrev;
  } static const Table[] = {
    { State::QUOTE, "'" },
    { State::QUASIQUOTE, "`" },
    { State::UNQUOTE, "," },
    { State::UNQUOTE_SPLICING, ",@" },
  };
  Value a = car();
  for (auto t : Table)
    if (a.eq(state->getConstant(t.c)))
      return t.abbrev;
  return NULL;
}

//=============================================================================

String::String(const char* string, int len)
  : Object()
  , string_(string), len_(len) {
}

void String::destruct(Allocator* allocator) {
  allocator->free(const_cast<char*>(string_));
  Object::destruct(allocator);
}

Type String::getType() const  { return TT_STRING; }

bool String::equal(const Object* target) const {
  const String* p = static_cast<const String*>(target);
  return len_ == p->len_ &&
    memcmp(string_, p->string_, len_) == 0;
}

unsigned int String::calcHash(State*) const {
  return strHash(string_);
}

void String::output(State*, Stream* o, bool inspect) const {
  if (!inspect) {
    o->write(string_, len_);
    return;
  }

  o->write('"');
  int prev = 0;
  for (int n = len_, i = 0; i < n; ++i) {
    unsigned char c = reinterpret_cast<const unsigned char*>(string_)[i];
    const char* s = NULL;
    switch (c) {
    case '\n':  s = "\\n"; break;
    case '\r':  s = "\\r"; break;
    case '\t':  s = "\\t"; break;
    case '\0':  s = "\\0"; break;
    case '\\':  s = "\\\\"; break;
    case '"':  s = "\\\""; break;
    }
    if (s == NULL)
      continue;

    if (prev != i)
      o->write(&string_[prev], i - prev);
    o->write(s);
    prev = i + 1;
  }
  if (prev != len_)
    o->write(&string_[prev], len_ - prev);
  o->write('"');
}

//=============================================================================

SFlonum::SFlonum(Flonum v)
  : Object()
  , v_(v) {
}

Type SFlonum::getType() const  { return TT_FLONUM; }

bool SFlonum::equal(const Object* target) const {
  const SFlonum* p = static_cast<const SFlonum*>(target);
  return v_ == p->v_;
}

void SFlonum::output(State*, Stream* o, bool) const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%f", v_);
  o->write(buffer);
}

//=============================================================================

Vector::Vector(Allocator* allocator, int size)
  : Object()
  , size_(size) {
  void* memory = allocator->alloc(sizeof(Value) * size_);
  buffer_ = new(memory) Value[size_];
}

void Vector::destruct(Allocator* allocator) {
  allocator->free(buffer_);
  Object::destruct(allocator);
}

Type Vector::getType() const { return TT_VECTOR; }

bool Vector::equal(const Object* target) const {
  const Vector* p = static_cast<const Vector*>(target);
  int n = size();
  if (n != p->size())
    return false;
  for (int i = 0; i < n; ++i) {
    if (!buffer_[i].equal(p->buffer_[i]))
      return false;
  }
  return true;
}

void Vector::output(State* state, Stream* o, bool inspect) const {
  o->write('#');
  char c = '(';
  for (int i = 0; i < size_; ++i) {
    o->write(c);
    buffer_[i].output(state, o, inspect);
    c = ' ';
  }
  o->write(')');
}

void Vector::mark() {
  if (isMarked())
    return;
  Object::mark();
  for (int n = size_, i = 0; i < n; ++i)
    buffer_[i].mark();
}

Value Vector::get(int index)  {
  assert(0 <= index && index < size_);
  return buffer_[index];
}

void Vector::set(int index, Value x)  {
  assert(0 <= index && index < size_);
  buffer_[index] = x;
}

//=============================================================================

SHashTable::SHashTable(Allocator* allocator, HashPolicy<Value>* policy)
  : Object() {
  void* memory = allocator->alloc(sizeof(*table_));
  table_ = new(memory) TableType(policy, allocator);
}

void SHashTable::destruct(Allocator* allocator) {
  table_->~TableType();
  allocator->free(table_);
  Object::destruct(allocator);
}

Type SHashTable::getType() const  { return TT_HASH_TABLE; }

void SHashTable::output(State*, Stream* o, bool) const {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "#<table:%p>", this);
  o->write(buffer);
}

void SHashTable::mark() {
  if (isMarked())
    return;
  Object::mark();
  TableType& table = *table_;
  for (auto kv : table)
    const_cast<Value*>(&kv.value)->mark();
}

int SHashTable::getCapacity() const  { return table_->getCapacity(); }
int SHashTable::getEntryCount() const  { return table_->getEntryCount(); }
int SHashTable::getConflictCount() const  { return table_->getConflictCount(); }
int SHashTable::getMaxDepth() const  { return table_->getMaxDepth(); }

void SHashTable::put(Value key, Value value) {
  table_->put(key, value);
}

const Value* SHashTable::get(Value key) const {
  return table_->get(key);
}

bool SHashTable::remove(Value key) {
  return table_->remove(key);
}

//=============================================================================
Callable::Callable()
  : Object()
  , name_(NULL)  {}

bool Callable::isCallable() const  { return true; }

void Callable::setName(const Symbol* name)  { name_ = name; }

//=============================================================================
// Closure class.
Closure::Closure(State* state, Value body, int freeVarCount, int minArgNum, int maxArgNum)
  : Callable()
  , body_(body), freeVariables_(NULL), freeVarCount_(freeVarCount)
  , minArgNum_(minArgNum), maxArgNum_(maxArgNum) {
  if (freeVarCount > 0) {
    void* memory = state->alloc(sizeof(Value) * freeVarCount);
    freeVariables_ = new(memory) Value[freeVarCount];
  }
}

void Closure::destruct(Allocator* allocator) {
  if (freeVariables_ != NULL)
    allocator->free(freeVariables_);
  Callable::destruct(allocator);
}

Type Closure::getType() const  { return TT_CLOSURE; }

void Closure::output(State*, Stream* o, bool) const {
  const char* name = "(noname)";
  if (name_ != NULL)
    name = name_->c_str();
  o->write("#<closure ");
  o->write(name);
  o->write('>');
}

void Closure::mark() {
  if (isMarked())
    return;
  Callable::mark();
  body_.mark();
  for (int n = freeVarCount_, i = 0; i < n; ++i)
    freeVariables_[i].mark();
}

//=============================================================================

NativeFunc::NativeFunc(NativeFuncType func, int minArgNum, int maxArgNum)
  : Callable()
  , func_(func)
  , minArgNum_(minArgNum)
  , maxArgNum_(maxArgNum) {}

Type NativeFunc::getType() const  { return TT_NATIVEFUNC; }

void NativeFunc::output(State*, Stream* o, bool) const {
  const char* name = "(noname)";
  if (name_ != NULL)
    name = name_->c_str();
  o->write("#<subr ");
  o->write(name);
  o->write('>');
}

//=============================================================================
Continuation::Continuation(State* state, const Value* stack, int size,
                           const CallStack* callStack, int callStackSize)
  : Callable(), copiedStack_(NULL), stackSize_(0)
  , callStack_(NULL), callStackSize_(0) {
  if (size > 0) {
    copiedStack_ = static_cast<Value*>(state->alloc(sizeof(Value) * size));
    stackSize_ = size;
    memcpy(copiedStack_, stack, sizeof(Value) * size);
  }

  if (callStackSize > 0) {
    int nbytes = sizeof(CallStack) * callStackSize;
    callStack_ = static_cast<CallStack*>(state->alloc(nbytes));
    callStackSize_ = callStackSize;
    memcpy(callStack_, callStack, nbytes);
  }
}

void Continuation::destruct(Allocator* allocator) {
  if (callStack_ != NULL)
    allocator->free(callStack_);
  if (copiedStack_ != NULL)
    allocator->free(copiedStack_);
  Callable::destruct(allocator);
}

Type Continuation::getType() const  { return TT_CONTINUATION; }

void Continuation::output(State*, Stream* o, bool) const {
  o->write("#<continuation>");
}

void Continuation::mark() {
  if (isMarked())
    return;
  Callable::mark();
  for (int n = stackSize_, i = 0; i < n; ++i)
    copiedStack_[i].mark();
}

//=============================================================================
SStream::SStream(Stream* stream)
  : Object(), stream_(stream)  {}

void SStream::destruct(Allocator* allocator) {
  allocator->free(stream_);
}

Type SStream::getType() const  { return TT_STREAM; }

void SStream::output(State*, Stream* o, bool) const {
  o->write("#<stream:");
  o->write('>');
}

//=============================================================================

}  // namespace yalp
