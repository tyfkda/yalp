//=============================================================================
/// YALP - Yet Another List Processor.
//=============================================================================

#include "build_env.hh"
#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/read.hh"
#include "yalp/stream.hh"
#include "yalp/util.hh"
#include "basic.hh"
#include "flonum.hh"
#include "symbol_manager.hh"
#include "vm.hh"

#include <assert.h>
#include <string.h>  // for strlen

namespace yalp {

//=============================================================================
/*
  Value: tagged pointer representation.
    XXXXXXX0 : Fixnum
    XXXXXX01 : Object
    XXXX0011 : Symbol
    XXXX0111 : Character
 */

#define TAG2_TYPE(x)  (((x) << TAG_SHIFT) | TAG_OTHER)
const Fixnum TAG2_SHIFT = 4;
const Fixnum TAG2_MASK = (1 << TAG2_SHIFT) - 1;
const Fixnum TAG2_SYMBOL = TAG2_TYPE(0);
const Fixnum TAG2_CHAR = TAG2_TYPE(1);

// Assumes that first symbol is nil.
const Value Value::NIL = Value(0, TAG2_SYMBOL);

Value::Value(Fixnum i, int tag2)
  : v_((i << TAG2_SHIFT) | tag2) {}

Type Value::getType() const {
  if (isFixnum())
    return TT_FIXNUM;

  switch (v_ & TAG_MASK) {
  case TAG_OBJECT:
    return toObject()->getType();
  case TAG_OTHER:
    switch (v_ & TAG2_MASK) {
    case TAG2_SYMBOL:
      return TT_SYMBOL;
    case TAG2_CHAR:
      return TT_CHAR;
    }
  }
  assert(!"Must not happen");
  return TT_UNKNOWN;
}

unsigned int Value::calcHash(State* state) const {
  if (isFixnum())
    return toFixnum() * 19;

  switch (v_ & TAG_MASK) {
  case TAG_OBJECT:
    return toObject()->calcHash(state);
  case TAG_OTHER:
    switch (v_ & TAG2_MASK) {
    case TAG2_SYMBOL:
      if (v_ >= 0)
        return toSymbol(state)->getHash();
      return (v_ >> TAG2_SHIFT) * 29;
    case TAG2_CHAR:
      return (v_ >> TAG2_SHIFT) * 41;
    }
  }
  assert(!"Must not happen");
  return 0;
}

void Value::mark() {
  if (isObject())
    toObject()->mark();
}

void Value::output(State* state, Stream* o, bool inspect) const {
  if (isFixnum()) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%ld", toFixnum());
    o->write(buffer);
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
      outputSymbol(state, o);
      break;
    case TAG2_CHAR:
      outputCharacter(o, inspect);
      break;
    }
  }
}

void Value::outputSymbol(State* state, Stream* o) const {
  char buffer[64];
  if (state == NULL) {
    snprintf(buffer, sizeof(buffer), "#<symbol:%ld>", v_ >> TAG2_SHIFT);
    o->write(buffer);
    return;
  }

  if (v_ < 0) {  // gensym-ed symbol.
    int id = -(v_ >> TAG2_SHIFT);
    snprintf(buffer, sizeof(buffer), "G:%d", id);
    o->write(buffer);
    return;
  }

  // Normal symbol.
  o->write(toSymbol(state)->c_str());
}

void Value::outputCharacter(Stream* o, bool inspect) const {
  Fixnum c = v_ >> TAG2_SHIFT;
  char buffer[16];
  if (inspect) {
    const char* str = NULL;
    switch (c) {
    case '\n':  str = "#\\nl"; break;
    case '\t':  str = "#\\tab"; break;
    case ' ':  str = "#\\space"; break;
    case 0x1b:  str = "#\\escape"; break;
    default:
      if (0x20 <= c && c < 0x80) {
        snprintf(buffer, sizeof(buffer), "#\\%c", static_cast<int>(c));
        o->write(buffer);
        return;
      }
      snprintf(buffer, sizeof(buffer), "#\\x%x", (int)c);
      str = buffer;
      break;
    }
    o->write(str);
    return;
  }
  o->write(static_cast<int>(c));
}

#ifndef DISABLE_FLONUM
Flonum Value::toFlonum(State* state) const {
  switch (getType()) {
  case TT_FLONUM:
    return static_cast<SFlonum*>(toObject())->toFlonum();
  case TT_FIXNUM:
    return static_cast<Flonum>(toFixnum());
  default:
    state->runtimeError("Flonum expected, but `%@`", this);
    return 0;
  }
}
#endif

const Symbol* Value::toSymbol(State* state) const {
  assert((v_ & TAG2_MASK) == TAG2_SYMBOL);
  return state->getSymbol(v_ >> TAG2_SHIFT);
}

int Value::toCharacter() const {
  switch (getType()) {
  case TT_FIXNUM:
    return static_cast<int>(toFixnum());
  case TT_CHAR:
    return v_ >> TAG2_SHIFT;
  default:
    return -1;
  }
}

bool Value::equal(Value target) const {
  if (eq(target))
    return true;

  if (isFixnum())
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
    case TAG2_CHAR:
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

struct State::HashPolicyEq : public HashPolicy<Value> {
  HashPolicyEq(State* state) : state_(state)  {}

  virtual unsigned int hash(Value a) override  { return a.calcHash(state_); }
  virtual bool equal(Value a, Value b) override  { return a.eq(b); }

private:
  State* state_;
};

struct State::HashPolicyEqual : public HashPolicy<Value> {
  HashPolicyEqual(State* state) : state_(state)  {}

  virtual unsigned int hash(Value a) override  { return a.calcHash(state_); }
  virtual bool equal(Value a, Value b) override  { return a.equal(b); }

private:
  State* state_;
};

State* State::create() {
  return create(getDefaultAllocFunc());
}

State* State::create(AllocFunc allocFunc) {
  Allocator* allocator = Allocator::create(allocFunc, &stateAllocatorCallback);
  void* memory = allocator->alloc(sizeof(State));
  return new(memory) State(allocator);
}

void State::release() {
  Allocator* allocator = allocator_;
  this->~State();
  allocator->free(this);
  allocator->release();
}

State::State(Allocator* allocator)
  : allocator_(allocator)
  , symbolManager_(SymbolManager::create(allocator_))
  , hashPolicyEq_(new(allocator_->alloc(sizeof(*hashPolicyEq_))) HashPolicyEq(this))
  , hashPolicyEqual_(new(allocator_->alloc(sizeof(*hashPolicyEqual_))) HashPolicyEqual(this))
  , readTable_(NULL), vm_(NULL), jmp_(NULL)
  , gensymIndex_(0) {
  int arena = saveArena();
  allocator->setUserData(this);

  intern("nil");  // "nil" must be the first symbol.
  static const char* constSymbols[NUMBER_OF_CONSTANTS] = {
    "t", "quote", "quasiquote", "unquote", "unquote-splicing", "compile",
    "*stdin*", "*stdout*",
  };
  for (int i = 0; i < NUMBER_OF_CONSTANTS; ++i)
    constants_[i] = intern(constSymbols[i]);

  static const char* TypeSymbolStrings[NUMBER_OF_TYPES] = {
    "unknown", "int", "symbol", "pair", "string", "char",
#ifndef DISABLE_FLONUM
    "flonum",
#endif
    "closure", "subr", "continuation", "vector", "table", "stream", "macro",
    "box",
  };
  for (int i = 0; i < NUMBER_OF_TYPES; ++i)
    typeSymbols_[i] = intern(TypeSymbolStrings[i]);

  vm_ = Vm::create(this);
  installBasicFunctions(this);
#ifndef DISABLE_FLONUM
  installFlonumFunctions(this);
#endif
  installBasicObjects();

  readTable_ = createHashTable(false);

  restoreArena(arena);
}

State::~State() {
  allocator_->free(hashPolicyEqual_);
  allocator_->free(hashPolicyEq_);
  vm_->release();
  symbolManager_->release();
}

void State::installBasicObjects() {
  struct {
    FILE* fp;
    const char* name;
  } static const Table[] = {
    { stdin, "*stdin*" },
    { stdout, "*stdout*" },
    { stderr, "*stderr*" },
  };

  int arena = saveArena();
  for (auto e : Table)
    defineGlobal(intern(e.name), Value(createFileStream(e.fp)));
  restoreArena(arena);
}

bool State::compile(Value exp, Value* pValue) {
  Value fn = referGlobal(getConstant(COMPILE));
  if (fn.isFalse()) {
    runtimeError("`compile` is not enabled");
    return false;
  }
  return funcall(fn, 1, &exp, pValue);
}

bool State::runBinary(Value code, Value* pResult) {
  jmp_buf* old = NULL;
  jmp_buf jmp;
  bool ret = false;
  if (setjmp(jmp) == 0) {
    old = setJmpbuf(&jmp);
    Value result = vm_->run(code);
    if (pResult != NULL)
      *pResult = result;
    ret = true;
  }
  setJmpbuf(old);
  return ret;
}

ErrorCode State::runFromFile(const char* filename, Value* pResult) {
  FileStream stream(filename, "r");
  if (!stream.isOpened())
    return FILE_NOT_FOUND;
  return run(&stream, pResult);
}

ErrorCode State::runFromString(const char* string, Value* pResult) {
  StrStream stream(string);
  return run(&stream, pResult);
}

ErrorCode State::run(Stream* stream, Value* pResult) {
  Reader reader(this, stream);
  for (;;) {
    int arena = allocator_->saveArena();
    Value exp;
    ErrorCode err = reader.read(&exp);
    if (err != SUCCESS) {
      if (err == END_OF_FILE)
        return SUCCESS;
      raiseReadError(this, err, &reader);
      return err;
    }
    Value code;
    if (!compile(exp, &code) || code.isFalse())
      return COMPILE_ERROR;
    if (!runBinary(code, pResult))
      return RUNTIME_ERROR;
    allocator_->restoreArena(arena);
  }
}

ErrorCode State::runBinaryFromFile(const char* filename, Value* pResult) {
  FileStream stream(filename, "r");
  if (!stream.isOpened())
    return FILE_NOT_FOUND;

  Reader reader(this, &stream);
  for (;;) {
    int arena = allocator_->saveArena();
    Value bin;
    ErrorCode err = reader.read(&bin);
    if (err != SUCCESS) {
      if (err == END_OF_FILE)
        return SUCCESS;
      raiseReadError(this, err, &reader);
      return err;
    }
    if (!runBinary(bin, pResult))
      return RUNTIME_ERROR;
    allocator_->restoreArena(arena);
  }
}

void State::checkType(Value x, Type expected) {
  if (x.getType() != expected) {
    Value expectedType = getTypeSymbol(expected);
    runtimeError("Type error, %@ expected, but `%@`", &expectedType, &x);
  }
}

void* State::alloc(size_t size) const {
  return allocator_->alloc(size);
}

void* State::realloc(void* ptr, size_t size) const {
  return allocator_->realloc(ptr, size);
}

void State::free(void* ptr) const {
  allocator_->free(ptr);
}

Value State::intern(const char* name) {
  SymbolId symbolId = symbolManager_->intern(name);
  return Value(symbolId, TAG2_SYMBOL);
}

Value State::gensym() {
  // Gensym'ed symbol has negative index for symbol.
  return Value(-(++gensymIndex_), TAG2_SYMBOL);
}

const Symbol* State::getSymbol(int symbolId) const {
  assert(symbolId >= 0);
  return symbolManager_->get(symbolId);
}

Value State::cons(Value a, Value d) {
  return Value(allocator_->newObject<Cell>(a, d));
}

Value State::character(int c) const {
  return Value(c, TAG2_CHAR);
}

SHashTable* State::createHashTable(bool equal) {
  HashPolicy<Value>* policy;
  if (equal)
    policy = hashPolicyEqual_;
  else
    policy = hashPolicyEq_;
  return allocator_->newObject<SHashTable>(allocator_, policy);
}

Value State::string(const char* str) {
  return string(str, strlen(str));
}

Value State::string(const char* str, size_t len) {
  void* stringBuffer = allocator_->alloc(sizeof(char) * (len + 1));
  char* copiedString = new(stringBuffer) char[len + 1];
  memcpy(copiedString, str, len);
  copiedString[len] = '\0';
  return allocatedString(copiedString, len);
}

Value State::allocatedString(const char* str, size_t len) {
  return Value(allocator_->newObject<String>(str, len));
}

Value State::createFileStream(FILE* fp) {
  void* memory = allocator_->alloc(sizeof(FileStream));
  FileStream* stream = new(memory) FileStream(fp);
  return Value(allocator_->newObject<SStream>(stream));
}

Object* State::getFunc() const {
  return vm_->getFunc();
}

int State::getArgNum() const {
  return vm_->getArgNum();
}

Value State::getArg(int index) const {
  return vm_->getArg(index);
}

Value State::multiValues() const  { return vm_->multiValues(); }
Value State::multiValues(Value v0) const  { return vm_->multiValues(v0); }
Value State::multiValues(Value v0, Value v1) const  { return vm_->multiValues(v0, v1); }
Value State::multiValues(Value v0, Value v1, Value v2) const  { return vm_->multiValues(v0, v1, v2); }

int State::getResultNum() const {
  return vm_->getResultNum();
}

Value State::getResult(int index) const {
  return vm_->getResult(index);
}

int State::saveArena() const  { return allocator_->saveArena(); }
void State::restoreArena(int index)  { allocator_->restoreArena(index); }
void State::restoreArenaWith(int index, Value v) {
  if (v.isObject())
    allocator_->restoreArenaWith(index, v.toObject());
  else
    allocator_->restoreArena(index);
}

void State::runtimeError(const char* msg, ...) {
  FileStream errout(stderr);
  va_list ap;
  va_start(ap, msg);
  format(this, &errout, msg, ap);
  errout.write('\n');
  va_end(ap);

  int n = vm_->getCallStackDepth();
  if (n > 0) {
    const CallStack* callStack = vm_->getCallStack();
    for (int i = n; --i >= 0; ) {
      const Symbol* name = callStack[i].callable->getName();
      format(this, &errout, "\tfrom %s\n", (name != NULL ? name->c_str() : "(noname)"));
    }
  }
  longJmp();
}

Value State::referGlobal(Value sym, bool* pExist) {
  return vm_->referGlobal(sym, pExist);
}

void State::defineGlobal(Value sym, Value value) {
  vm_->defineGlobal(sym, value);
}

void State::defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum) {
  int arena = saveArena();
  NativeFunc* nativeFunc = allocator_->newObject<NativeFunc>(func, minArgNum, maxArgNum);
  defineGlobal(name, Value(nativeFunc));
  restoreArena(arena);
}

SHashTable* State::getGlobalVariableTable() const {
  return vm_->getGlobalVariableTable();
}

void State::setMacroCharacter(int c, Value func) {
  readTable_->put(character(c), func);
}

Value State::getMacroCharacter(int c) {
  const Value* p = readTable_->get(character(c));
  return (p != NULL) ? *p : Value::NIL;
}

Value State::getMacro(Value name) {
  return vm_->getMacro(name);
}

bool State::funcall(Value fn, int argNum, const Value* args, Value* pResult) {
  jmp_buf* old = NULL;
  jmp_buf jmp;
  bool ret = false;
  if (setjmp(jmp) == 0) {
    old = setJmpbuf(&jmp);
    Value result = vm_->funcall(fn, argNum, args);
    if (pResult != NULL)
      *pResult = result;
    ret = true;
  }
  setJmpbuf(old);
  return ret;
}

Value State::tailcall(Value fn, int argNum, const Value* args) {
  return vm_->tailcall(fn, argNum, args);
}

Value State::applyFunction() {
  return vm_->applyFunction();
}

jmp_buf* State::setJmpbuf(jmp_buf* jmp) {
  jmp_buf* old = jmp_;
  jmp_ = jmp;
  return old;
}

void State::longJmp() {
  if (jmp_ != NULL)
    longjmp(*jmp_, 1);
}

void State::resetError() {
  vm_->resetError();
}

void State::collectGarbage() {
  allocator_->collectGarbage();
}

void State::setVmTrace(bool b) {
  vm_->setTrace(b);
}

void State::markRoot() {
  readTable_->mark();
  vm_->markRoot();
}

void State::allocFailed(void*, size_t) {
  runtimeError("allocFailed");
}

void State::reportDebugInfo() const {
  vm_->reportDebugInfo();
  symbolManager_->reportDebugInfo();
  fprintf(stdout, "Total gensym: %d\n", gensymIndex_);
  fprintf(stdout, "Max arena index: %d\n", allocator_->getMaxArenaIndex());
}

}  // namespace yalp
