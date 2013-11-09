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
  Svalue run(Svalue a, Svalue code, int f, Svalue c, int s);

  State* state_;
  Svalue* stack_;
  int stackSize_;
};

}  // namespace macp

#endif
