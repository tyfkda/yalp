//=============================================================================
/// Vm - Macro Processor VM.
//=============================================================================

#ifndef _VM_HH_
#define _VM_HH_

namespace macp {

class State;
class Svalue;

// Vm class.
class Vm {
public:
  static Vm* create(State* state);
  virtual ~Vm();

  Svalue run(Svalue code);

private:
  Vm(State* state);
  Svalue run(Svalue a, Svalue x, int f, Svalue c, int s);
  int findOpcode(Svalue op);
  Svalue createClosure(Svalue body, int n, int s);

  int push(Svalue x, int s);
  Svalue index(int s, int i);
  void expandStack();

  State* state_;
  Svalue* stack_;
  int stackSize_;

  // Symbols
  Svalue* opcodes_;
};

}  // namespace macp

#endif
