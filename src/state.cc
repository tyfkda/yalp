//=============================================================================
/// YALP - Yet Another List Processor.
//=============================================================================

#include "yalp.hh"
#include "yalp/mem.hh"
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "basic.hh"
#include "symbol_manager.hh"
#include "vm.hh"
#include <assert.h>
#include <fstream>
#include <iostream>

namespace yalp {

const Sfixnum TAG_SHIFT = 2;
const Sfixnum TAG_MASK = (1 << TAG_SHIFT) - 1;
const Sfixnum TAG_FIXNUM = 0;
const Sfixnum TAG_OBJECT = 1;

//=============================================================================
Svalue::Svalue() : v_(TAG_OBJECT) {
  // Initialized to illegal value.
}

Svalue::Svalue(Sfixnum i)
  : v_(reinterpret_cast<Sfixnum>(i << TAG_SHIFT) | TAG_FIXNUM) {}

Svalue::Svalue(class Sobject* object)
  : v_(reinterpret_cast<Sfixnum>(object) | TAG_OBJECT) {}

Type Svalue::getType() const {
  switch (v_ & TAG_MASK) {
  default:
    assert(false);
    return TT_UNKNOWN;
  case TAG_FIXNUM:
    return TT_FIXNUM;
  case TAG_OBJECT:
    return toObject()->getType();
  }
}

void Svalue::output(State* state, std::ostream& o, bool inspect) const {
  switch (v_ & TAG_MASK) {
  default:
    assert(false);
    break;
  case TAG_FIXNUM:
    o << toFixnum();
    break;
  case TAG_OBJECT:
    toObject()->output(state, o, inspect);
    break;
  }
}

Sfixnum Svalue::toFixnum() const {
  assert((v_ & TAG_MASK) == TAG_FIXNUM);
  assert(TAG_FIXNUM == 0);
  return reinterpret_cast<Sfixnum>(v_ >> TAG_SHIFT);
}

Sobject* Svalue::toObject() const {
  assert((v_ & TAG_MASK) == TAG_OBJECT);
  return reinterpret_cast<Sobject*>(v_ & ~TAG_OBJECT);
}

bool Svalue::equal(Svalue target) const {
  switch (v_ & TAG_MASK) {
  default:
    assert(false);
    return false;
  case TAG_FIXNUM:
    return eq(target);
  case TAG_OBJECT:
    {
      Type t1 = getType();
      Type t2 = target.getType();
      if (t1 != t2)
        return false;

      return toObject()->equal(target.toObject());
    }
  }
}

//=============================================================================
State* State::create() {
  return create(Allocator::getDefaultAllocator());
}

State* State::create(Allocator* allocator) {
  void* memory = allocator->alloc(sizeof(State));
  return new(memory) State(allocator);
}

void State::release() {
  Allocator* allocator = allocator_;
  this->~State();
  allocator->free(this);
}

State::State(Allocator* allocator)
  : allocator_(allocator)
  , symbolManager_(SymbolManager::create(allocator))
  , nil_(symbolManager_->intern("nil"))
  , t_(symbolManager_->intern("t"))
  , quote_(symbolManager_->intern("quote"))
  , vm_(NULL) {
  vm_ = Vm::create(this);
  installBasicFunctions(this);
}

State::~State() {
  symbolManager_->release();
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
  return Svalue(symbolManager_->intern(name));
}

Svalue State::gensym() {
  return Svalue(symbolManager_->gensym());
}

Svalue State::cons(Svalue a, Svalue d) {
  void* memory = allocator_->alloc(sizeof(Cell));
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

Svalue State::quote(Svalue x) {
  return list(this, quote_, x);
}

Svalue State::createHashTable() {
  void* memory = allocator_->alloc(sizeof(SHashTable));
  SHashTable* h = new(memory) SHashTable(allocator_);
  return Svalue(h);
}

Svalue State::stringValue(const char* string) {
  int len = strlen(string);
  void* stringBuffer = allocator_->alloc(sizeof(char) * (len + 1));
  char* copiedString = new(stringBuffer) char[len + 1];
  strcpy(copiedString, string);

  void* memory = allocator_->alloc(sizeof(String));
  String* s = new(memory) String(copiedString);
  return Svalue(s);
}

int State::getArgNum() const {
  return vm_->getArgNum();
}

Svalue State::getArg(int index) const {
  return vm_->getArg(index);
}

void State::runtimeError(const char* msg) {
  std::cerr << msg << std::endl;
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
