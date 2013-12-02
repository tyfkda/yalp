//=============================================================================
/// YALP - Yet Another List Processor.
//=============================================================================

#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "basic.hh"
#include "symbol_manager.hh"
#include "vm.hh"
#include <assert.h>
#include <fstream>
#include <iostream>

namespace yalp {

//=============================================================================
/*
  Svalue: tagged pointer representation.
    XXXXXXX0 : Fixnum
    XXXXXX01 : Object
    XXXX0011 : Symbol
 */

const Sfixnum TAG_SHIFT = 2;
const Sfixnum TAG_MASK = (1 << TAG_SHIFT) - 1;
const Sfixnum TAG_FIXNUM = 0;
const Sfixnum TAG_OBJECT = 1;
const Sfixnum TAG_OTHER = 3;

const Sfixnum TAG2_SHIFT = 4;
const Sfixnum TAG2_MASK = (1 << TAG2_SHIFT) - 1;
const Sfixnum TAG2_SYMBOL = 3;

inline static bool isFixnum(Sfixnum v)  { return (v & 1) == TAG_FIXNUM; }

Svalue::Svalue() : v_(TAG_OBJECT) {
  // Initialized to illegal value.
}

Svalue::Svalue(Sfixnum i)
  : v_(reinterpret_cast<Sfixnum>(i << 1) | TAG_FIXNUM) {}

Svalue::Svalue(class Sobject* object)
  : v_(reinterpret_cast<Sfixnum>(object) | TAG_OBJECT) {}

Svalue::Svalue(Sfixnum i, int tag2)
  : v_(reinterpret_cast<Sfixnum>(i << TAG2_SHIFT) | tag2) {}

Type Svalue::getType() const {
  if (isFixnum(v_))
    return TT_FIXNUM;

  switch (v_ & TAG_MASK) {
  case TAG_OBJECT:
    return toObject()->getType();
  case TAG_OTHER:
    switch (v_ & TAG2_MASK) {
    case TAG2_SYMBOL:
      return TT_SYMBOL;
    }
  }
  assert(!"Must not happen");
  return TT_UNKNOWN;
}

unsigned int Svalue::calcHash(State* state) const {
  if (isFixnum(v_))
    return toFixnum() * 19;

  switch (v_ & TAG_MASK) {
  case TAG_OBJECT:
    return toObject()->calcHash();
  case TAG_OTHER:
    switch (v_ & TAG2_MASK) {
    case TAG2_SYMBOL:
      return toSymbol(state)->calcHash();
      break;
    }
  }
  assert(!"Must not happen");
  return 0;
}

void Svalue::mark() {
  if (isObject())
    toObject()->mark();
}

void Svalue::output(State* state, std::ostream& o, bool inspect) const {
  if (isFixnum(v_)) {
    o << toFixnum();
    return;
  }

  switch (v_ & TAG_MASK) {
  default:
    assert(false);
    break;
  case TAG_OBJECT:
    toObject()->output(state, o, inspect);
    break;
  case TAG_OTHER:
    switch (v_ & TAG2_MASK) {
    case TAG2_SYMBOL:
      return toSymbol(state)->output(state, o, inspect);
      break;
    }
  }
}

Sfixnum Svalue::toFixnum() const {
  assert(isFixnum(v_));
  return reinterpret_cast<Sfixnum>(v_ >> 1);
}

Sfloat Svalue::toFloat() const {
  assert(getType() == TT_FLOAT);
  return static_cast<Float*>(toObject())->toFloat();
}

bool Svalue::isObject() const {
  return (v_ & TAG_MASK) == TAG_OBJECT;
}

Sobject* Svalue::toObject() const {
  assert(isObject());
  return reinterpret_cast<Sobject*>(v_ & ~TAG_OBJECT);
}

const Symbol* Svalue::toSymbol(State* state) const {
  assert((v_ & TAG2_MASK) == TAG2_SYMBOL);
  return state->getSymbol(v_ >> TAG2_SHIFT);
}

bool Svalue::equal(Svalue target) const {
  if (eq(target))
    return true;

  if (isFixnum(v_))
    return false;

  switch (v_ & TAG_MASK) {
  case TAG_OBJECT:
    {
      Type t1 = getType();
      Type t2 = target.getType();
      if (t1 != t2)
        return false;

      return toObject()->equal(target.toObject());
    }
  case TAG_OTHER:
    switch (v_ & TAG2_MASK) {
    case TAG2_SYMBOL:
      return false;
    }
  }
  assert(!"Must not happen");
  return false;
}

//=============================================================================
struct StateAllocatorCallback : public Allocator::Callback {
  virtual void allocFailed(void* p, size_t size, void* userdata) override {
    State* state = static_cast<State*>(userdata);
    state->allocFailed(p, size);
  }
  virtual void markRoot(void* userdata) override {
    State* state = static_cast<State*>(userdata);
    state->markRoot();
  }
};

static StateAllocatorCallback stateAllocatorCallback;

struct State::HashPolicyEq : public HashPolicy<Svalue> {
  HashPolicyEq(State* state) : state_(state)  {}

  virtual unsigned int hash(const Svalue a) override  { return a.calcHash(state_); }
  virtual bool equal(const Svalue a, const Svalue b) override  { return a.eq(b); }

private:
  State* state_;
};

State* State::create() {
  return create(getDefaultAllocFunc());
}

State* State::create(AllocFunc allocFunc) {
  void* memory = RAW_ALLOC(allocFunc, sizeof(State));
  return new(memory) State(allocFunc);
}

void State::release() {
  AllocFunc allocFunc = allocFunc_;
  this->~State();
  RAW_FREE(allocFunc, this);
}

State::State(AllocFunc allocFunc)
  : allocFunc_(allocFunc)
  , allocator_(Allocator::create(allocFunc, &stateAllocatorCallback, this))
  , symbolManager_(SymbolManager::create(allocator_))
  , hashPolicyEq_(new(ALLOC(allocator_, sizeof(*hashPolicyEq_))) HashPolicyEq(this))
  , vm_(NULL) {
  static const char* constSymbols[SINGLE_HALT] = {
    "nil", "t", "quote", "quasiquote", "unquote", "unquote-splicing"
  };
  for (int i = 0; i < SINGLE_HALT; ++i)
    constant_[i] = intern(constSymbols[i]);
  constant_[SINGLE_HALT] = list(this, intern("HALT"));

  vm_ = Vm::create(this);
  installBasicFunctions(this);
}

State::~State() {
  FREE(allocator_, hashPolicyEq_);
  vm_->release();
  symbolManager_->release();
  allocator_->release();
}

bool State::compile(Svalue exp, Svalue* pValue) {
  Svalue fn = referGlobal(intern("compile"));
  if (fn.eq(nil()))
    return false;
  *pValue = funcall(fn, 1, &exp);
  return true;
}

Svalue State::runBinary(Svalue code) {
  return vm_->run(code);
}

bool State::runFromFile(const char* filename, Svalue* pResult) {
  std::ifstream strm(filename);
  if (!strm.is_open()) {
    std::cerr << "File not found: " << filename << std::endl;
    return false;
  }

  Reader reader(this, strm);
  Svalue result;
  Svalue exp;
  ReadError err;
  while ((err = reader.read(&exp)) == READ_SUCCESS) {
    Svalue code;
    if (!compile(exp, &code))
      return false;
    result = runBinary(code);
  }
  if (err != END_OF_FILE) {
    std::cerr << "Read error: " << err << std::endl;
    return false;
  }
  if (pResult != NULL)
    *pResult = result;
  return true;
}

bool State::runBinaryFromFile(const char* filename, Svalue* pResult) {
  std::ifstream strm(filename);
  if (!strm.is_open()) {
    std::cerr << "File not found: " << filename << std::endl;
    return false;
  }

  Reader reader(this, strm);
  Svalue result;
  Svalue bin;
  ReadError err;
  while ((err = reader.read(&bin)) == READ_SUCCESS) {
    result = runBinary(bin);
  }
  if (err != END_OF_FILE) {
    std::cerr << "Read error: " << err << std::endl;
    return false;
  }
  if (pResult != NULL)
    *pResult = result;
  return true;
}

Svalue State::intern(const char* name) {
  SymbolId symbolId = symbolManager_->intern(name);
  return Svalue(symbolId, TAG2_SYMBOL);
}

Svalue State::gensym() {
  return Svalue(symbolManager_->gensym(), TAG2_SYMBOL);
}

const Symbol* State::getSymbol(unsigned int symbolId) const {
  return symbolManager_->get(symbolId);
}

Svalue State::cons(Svalue a, Svalue d) {
  void* memory = OBJALLOC(allocator_, sizeof(Cell));
  Cell* cell = new(memory) Cell(a, d);
  return Svalue(cell);
}

Svalue State::car(Svalue s) {
  if (s.getType() != TT_CELL)
    runtimeError("Cell expected");
  return static_cast<Cell*>(s.toObject())->car();
}

Svalue State::cdr(Svalue s) {
  if (s.getType() != TT_CELL)
    runtimeError("Cell expected");
  return static_cast<Cell*>(s.toObject())->cdr();
}

Svalue State::createHashTable() {
  void* memory = OBJALLOC(allocator_, sizeof(SHashTable));
  SHashTable* h = new(memory) SHashTable(allocator_, hashPolicyEq_);
  return Svalue(h);
}

Svalue State::stringValue(const char* string) {
  int len = strlen(string);
  void* stringBuffer = ALLOC(allocator_, sizeof(char) * (len + 1));
  char* copiedString = new(stringBuffer) char[len + 1];
  strcpy(copiedString, string);

  void* memory = OBJALLOC(allocator_, sizeof(String));
  String* s = new(memory) String(copiedString);
  return Svalue(s);
}

Svalue State::floatValue(Sfloat f) {
  void* memory = OBJALLOC(allocator_, sizeof(Float));
  Float* p = new(memory) Float(f);
  return Svalue(p);
}

int State::getArgNum() const {
  return vm_->getArgNum();
}

Svalue State::getArg(int index) const {
  return vm_->getArg(index);
}

void State::runtimeError(const char* msg) {
  std::cerr << msg << std::endl;

  const Vm::CallStack* callStack = vm_->getCallStack();
  for (int n = vm_->getCallStackDepth(), i = n; --i >= 0; ) {
    const Symbol* name = callStack[i].callable->getName();
    std::cerr << "\tfrom " << (name != NULL ? name->c_str() : "_noname_") << std::endl;
  }

  exit(1);
}

Svalue State::referGlobal(Svalue sym, bool* pExist) {
  return vm_->referGlobal(sym, pExist);
}

void State::assignGlobal(Svalue sym, Svalue value) {
  vm_->assignGlobal(sym, value);
}

void State::assignNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum) {
  vm_->assignNative(name, func, minArgNum, maxArgNum);
}

Svalue State::funcall(Svalue fn, int argNum, const Svalue* args) {
  return vm_->funcall(fn, argNum, args);
}

void State::collectGarbage() {
  allocator_->collectGarbage();
}

void State::markRoot() {
  for (int i = SINGLE_HALT; i < NUMBER_OF_CONSTANT; ++i)
    constant_[i].mark();

  vm_->markRoot();
}

void State::allocFailed(void*, size_t) {
  runtimeError("allocFailed");
}

void State::reportDebugInfo() const {
  vm_->reportDebugInfo();
  symbolManager_->reportDebugInfo();
}

//=============================================================================

Svalue list(State* state, Svalue v1) {
  return state->cons(v1, state->nil());
}

Svalue list(State* state, Svalue v1, Svalue v2) {
  return state->cons(v1, state->cons(v2, state->nil()));
}

Svalue list(State* state, Svalue v1, Svalue v2, Svalue v3) {
  return state->cons(v1, state->cons(v2, state->cons(v3, state->nil())));
}

Svalue nreverse(State* state, Svalue v) {
  if (v.getType() != TT_CELL)
    return v;

  Svalue tail = state->nil();
  for (;;) {
    Cell* cell = static_cast<Cell*>(v.toObject());
    Svalue d = cell->cdr();
    cell->setCdr(tail);
    if (d.getType() != TT_CELL)
      return v;
    tail = v;
    v = d;
  }
}

int length(State* state, Svalue v) {
  int len = 0;
  for (; v.getType() == TT_CELL; v = state->cdr(v))
    ++len;
  return len;
}

//=============================================================================

}  // namespace yalp
