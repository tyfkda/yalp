//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#ifndef _VM_HH_
#define _VM_HH_

#include <map>

namespace yalp {

class State;
class Svalue;

typedef Svalue (*NativeFuncType)(State* state);

// Vm class.
class Vm {
public:
  static Vm* create(State* state);
  void release();

  // Execute compiled code.
  Svalue run(Svalue code);

  // Gets argument number for current native function.
  int getArgNum() const;
  // Gets argument value for the index.
  Svalue getArg(int index) const;

private:
  Vm(State* state);
  ~Vm();
  void installNativeFunctions();
  Svalue run(Svalue a, Svalue x, int f, Svalue c, int s);
  int findOpcode(Svalue op);
  Svalue createClosure(Svalue body, int n, int s);
  Svalue createContinuation(int s);
  Svalue box(Svalue x);

  int push(Svalue x, int s);
  Svalue index(int s, int i) const;
  void indexSet(int s, int i, Svalue v);
  void expandStack();
  int shiftArgs(int n, int m, int s);

  bool referGlobal(Svalue sym, Svalue* pValue);
  void assignGlobal(Svalue sym, Svalue value);
  void assignNative(const char* name, NativeFuncType func, int minArgNum) {
    assignNative(name, func, minArgNum, minArgNum);
  }
  void assignNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);

  Svalue saveStack(int s);
  int restoreStack(Svalue v);

  State* state_;
  Svalue* stack_;
  int stackSize_;

  // Symbols
  Svalue* opcodes_;

  // Global variables
  std::map<long, Svalue> globalVariableTable_;

  // For native function call.
  int stackPointer_;
  int argNum_;
};

}  // namespace yalp

#endif
