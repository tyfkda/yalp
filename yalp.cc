//=============================================================================
/// YALP - Yet Another List Processor.
//=============================================================================

#include "yalp.hh"
#include "object.hh"
#include "symbol_manager.hh"
#include "vm.hh"
#include <iostream>

namespace yalp {

//=============================================================================
State* State::create() {
  return new State;
}

State::State()
  : symbolManager_(new SymbolManager())
  , nil_(symbolManager_->intern("nil"))
  , t_(symbolManager_->intern("t"))
  , quote_(symbolManager_->intern("quote"))
  , vm_(NULL) {
  vm_ = Vm::create(this);
}

State::~State() {
  delete symbolManager_;
}

Svalue State::runBinary(Svalue code) {
  return vm_->run(code);
}

Svalue State::intern(const char* name) {
  return Svalue(symbolManager_->intern(name));
}

Svalue State::cons(Svalue a, Svalue d) {
  Cell* cell = new Cell(a, d);
  return Svalue(cell);
}

Svalue State::quote(Svalue x) {
  return list2(this, quote_, x);
}

Svalue State::stringValue(const char* string) {
  return Svalue(new String(string));
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

//=============================================================================

Svalue list1(State* state, Svalue v1) {
  return state->cons(v1, state->nil());
}

Svalue list2(State* state, Svalue v1, Svalue v2) {
  return state->cons(v1, state->cons(v2, state->nil()));
}

Svalue list3(State* state, Svalue v1, Svalue v2, Svalue v3) {
  return state->cons(v1, state->cons(v2, state->cons(v3, state->nil())));
}

Svalue nreverse(State* state, Svalue v) {
  if (v.getType() != TT_CELL)
    return v;

  Svalue tail = state->nil();
  for (;;) {
    Cell* cell = static_cast<Cell*>(v.toObject());
    Svalue d = cell->cdr();
    cell->rplacd(tail);
    if (d.getType() != TT_CELL)
      return v;
    tail = v;
    v = d;
  }
}

//=============================================================================

}  // namespace yalp
