//=============================================================================
/// Vm - Macro Processor VM.
//=============================================================================

#include "vm.hh"
#include "macp.hh"
#include <assert.h>
#include <iostream>

namespace macp {

enum Opcode {
  HALT,
  CONSTANT,

  NUMBER_OF_OPCODE
};

#define CELL(x)  (static_cast<Cell*>(x.toObject()))
#define CAR(x)  (CELL(x)->car())
#define CDR(x)  (CELL(x)->cdr())
#define CADR(x)  (CAR(CDR(x)))
#define CDDR(x)  (CDR(CDR(x)))
#define CADDR(x)  (CAR(CDDR(x)))
#define CDDDR(x)  (CDR(CDDR(x)))

Vm* Vm::create(State* state) {
  return new Vm(state);
}

Vm::~Vm() {
  delete[] opcodes_;
}

Vm::Vm(State* state)
  : state_(state)
  , stack_(NULL), stackSize_(0) {

  opcodes_ = new Svalue[NUMBER_OF_OPCODE];
  opcodes_[HALT] = state_->intern("HALT");
  opcodes_[CONSTANT] = state_->intern("CONSTANT");
}

Svalue Vm::run(Svalue code) {
  Svalue nil = state_->nil();
  return run(nil, code, 0, nil, 0);
}

Svalue Vm::run(Svalue a, Svalue x, int f, Svalue c, int s) {
 again:
  Svalue op = CAR(x);
  int opidx = findOpcode(op);
  if (opidx < 0) {
    std::cerr << "Unknown op: " << op << std::endl;
    return op;
  }

  switch (opidx) {
  case HALT:
    return a;
  case CONSTANT:
    a = CADR(x);
    x = CADDR(x);
    goto again;
  default:
    assert(false);
  }
}

int Vm::findOpcode(Svalue op) {
  for (int i = 0; i < NUMBER_OF_OPCODE; ++i)
    if (op.eq(opcodes_[i]))
      return i;
  return -1;
}

}  // namespace macp
