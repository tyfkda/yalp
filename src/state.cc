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
 */

const Fixnum TAG_SHIFT = 2;
const Fixnum TAG_MASK = (1 << TAG_SHIFT) - 1;
const Fixnum TAG_FIXNUM = 0;
const Fixnum TAG_OBJECT = 1;
const Fixnum TAG_OTHER = 3;

const Fixnum TAG2_SHIFT = 4;
const Fixnum TAG2_MASK = (1 << TAG2_SHIFT) - 1;
const Fixnum TAG2_SYMBOL = 3;

inline static bool isFixnum(Fixnum v)  { return (v & 1) == TAG_FIXNUM; }

// Assumes that first symbol is nil.
const Value Value::NIL = Value(0, TAG2_SYMBOL);

Value::Value() : v_(TAG_OBJECT) {
  // Initialized to illegal value.
}

Value::Value(Fixnum i)
  : v_((i << 1) | TAG_FIXNUM) {}

Value::Value(class Object* object)
  : v_(reinterpret_cast<Fixnum>(object) | TAG_OBJECT) {}

Value::Value(Fixnum i, int tag2)
  : v_((i << TAG2_SHIFT) | tag2) {}

Type Value::getType() const {
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

unsigned int Value::calcHash(State* state) const {
  if (isFixnum(v_))
    return toFixnum() * 19;

  switch (v_ & TAG_MASK) {
  case TAG_OBJECT:
    return toObject()->calcHash(state);
  case TAG_OTHER:
    switch (v_ & TAG2_MASK) {
    case TAG2_SYMBOL:
      return toSymbol(state)->getHash();
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
  if (isFixnum(v_)) {
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
      if (state != NULL)
        o->write(toSymbol(state)->c_str());
      else {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "#symbol:%ld>", v_ >> TAG2_SHIFT);
        o->write(buffer);
      }
      return;
    }
  }
}

Fixnum Value::toFixnum() const {
  assert(isFixnum(v_));
  return v_ >> 1;
}

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

bool Value::isObject() const {
  return (v_ & TAG_MASK) == TAG_OBJECT;
}

Object* Value::toObject() const {
  assert(isObject());
  return reinterpret_cast<Object*>(v_ & ~TAG_OBJECT);
}

const Symbol* Value::toSymbol(State* state) const {
  assert((v_ & TAG2_MASK) == TAG2_SYMBOL);
  return state->getSymbol(v_ >> TAG2_SHIFT);
}

bool Value::equal(Value target) const {
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
  , readTable_(NULL)
  , vm_(NULL)
  , jmp_(NULL) {

  int arena = saveArena();
  allocator->setUserData(this);

  intern("nil");  // "nil" must be the first symbol.
  static const char* constSymbols[NUMBER_OF_CONSTANTS] = {
    "t", "quote", "quasiquote", "unquote", "unquote-splicing", "compile"
  };
  for (int i = 0; i < NUMBER_OF_CONSTANTS; ++i)
    constants_[i] = intern(constSymbols[i]);

  static const char* TypeSymbolStrings[NUMBER_OF_TYPES] = {
    "unknown", "int", "symbol", "pair", "string", "flonum", "closure",
    "subr", "continuation", "vector", "table", "stream", "box",
  };
  for (int i = 0; i < NUMBER_OF_TYPES; ++i)
    typeSymbols_[i] = intern(TypeSymbolStrings[i]);

  vm_ = Vm::create(this);
  installBasicFunctions(this);
  installBasicObjects();

  {
    Value ht = createHashTable(false);
    assert(ht.getType() == TT_HASH_TABLE);
    readTable_ = static_cast<SHashTable*>(ht.toObject());
  }
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
  Value exp;
  ErrorCode err;
  for (;;) {
    int arena = allocator_->saveArena();
    err = reader.read(&exp);
    if (err != SUCCESS)
      break;
    Value code;
    if (!compile(exp, &code) || code.isFalse())
      return COMPILE_ERROR;
    if (!runBinary(code, pResult))
      return RUNTIME_ERROR;
    allocator_->restoreArena(arena);
  }
  if (err == END_OF_FILE)
    return SUCCESS;
  return err;
}

ErrorCode State::runBinaryFromFile(const char* filename, Value* pResult) {
  FileStream stream(filename, "r");
  if (!stream.isOpened())
    return FILE_NOT_FOUND;

  Reader reader(this, &stream);
  Value bin;
  ErrorCode err;
  for (;;) {
    int arena = allocator_->saveArena();
    err = reader.read(&bin);
    if (err != SUCCESS)
      break;
    if (!runBinary(bin, pResult))
      return RUNTIME_ERROR;
    allocator_->restoreArena(arena);
  }
  if (err == END_OF_FILE)
    return SUCCESS;
  return err;
}

void State::checkType(Value x, Type expected) {
  if (x.getType() != expected)
    runtimeError("Type error, %d expected, but `%@`", expected, &x);
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

void* State::objAlloc(size_t size) const {
  return allocator_->objAlloc(size);
}

Value State::intern(const char* name) {
  SymbolId symbolId = symbolManager_->intern(name);
  return Value(symbolId, TAG2_SYMBOL);
}

Value State::gensym() {
  return Value(symbolManager_->gensym(), TAG2_SYMBOL);
}

const Symbol* State::getSymbol(unsigned int symbolId) const {
  return symbolManager_->get(symbolId);
}

Value State::cons(Value a, Value d) {
  void* memory = allocator_->objAlloc(sizeof(Cell));
  Cell* cell = new(memory) Cell(a, d);
  return Value(cell);
}

Value State::car(Value s) {
  return s.getType() == TT_CELL ?
    static_cast<Cell*>(s.toObject())->car() : s;
}

Value State::cdr(Value s) {
  return s.getType() == TT_CELL ?
    static_cast<Cell*>(s.toObject())->cdr() : Value::NIL;
}

Value State::createHashTable(bool iso) {
  void* memory = allocator_->objAlloc(sizeof(SHashTable));
  SHashTable* h;
  if (iso)
    h = new(memory) SHashTable(allocator_, hashPolicyEqual_);
  else
    h = new(memory) SHashTable(allocator_, hashPolicyEq_);
  return Value(h);
}

Value State::string(const char* str) {
  return string(str, strlen(str));
}

Value State::string(const char* str, int len) {
  void* stringBuffer = allocator_->alloc(sizeof(char) * (len + 1));
  char* copiedString = new(stringBuffer) char[len + 1];
  memcpy(copiedString, str, len);
  copiedString[len] = '\0';
  return allocatedString(copiedString, len);
}

Value State::allocatedString(const char* str, int len) {
  void* memory = allocator_->objAlloc(sizeof(String));
  String* s = new(memory) String(str, len);
  return Value(s);
}


Value State::flonum(Flonum f) {
  void* memory = allocator_->objAlloc(sizeof(SFlonum));
  SFlonum* p = new(memory) SFlonum(f);
  return Value(p);
}

Value State::createFileStream(FILE* fp) {
  void* memory = allocator_->alloc(sizeof(FileStream));
  FileStream* stream = new(memory) FileStream(fp);
  void* memory2 = allocator_->objAlloc(sizeof(SStream));
  return Value(new(memory2) SStream(stream));
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
  void* memory = objAlloc(sizeof(NativeFunc));
  NativeFunc* nativeFunc = new(memory) NativeFunc(func, minArgNum, maxArgNum);
  defineGlobal(name, Value(nativeFunc));
  restoreArena(arena);
}

void State::defineNativeMacro(const char* name, NativeFuncType func, int minArgNum, int maxArgNum) {
  int arena = saveArena();
  void* memory = objAlloc(sizeof(NativeFunc));
  NativeFunc* nativeFunc = new(memory) NativeFunc(func, minArgNum, maxArgNum);
  vm_->defineMacro(intern(name), Value(nativeFunc));
  restoreArena(arena);
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

jmp_buf* State::setJmpbuf(jmp_buf* jmp) {
  jmp_buf* old = jmp_;
  jmp_ = jmp;
  return old;
}

void State::longJmp() {
  if (jmp_ != NULL)
    longjmp(*jmp_, 1);

  // If process comes here, something wrong.
  FileStream errout(stderr);
  errout.write("State::longJmp is empty\n");
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
}

}  // namespace yalp
