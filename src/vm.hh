//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#ifndef _VM_HH_
#define _VM_HH_

#include "yalp.hh"

namespace yalp {

class SHashTable;

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

  Svalue referGlobal(Svalue sym, bool* pExist);
  void assignGlobal(Svalue sym, Svalue value);
  void assignNative(const char* name, NativeFuncType func, int minArgNum) {
    assignNative(name, func, minArgNum, minArgNum);
  }
  void assignNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);

  Svalue funcall(Svalue fn, int argNum, const Svalue* args);

  void reportDebugInfo() const;

private:
  Vm(State* state);
  ~Vm();
  void installNativeFunctions();
  Svalue run(Svalue a, Svalue x, int f, Svalue c, int s);
  int findOpcode(Svalue op);
  Svalue createClosure(Svalue body, int nfree, int s, int minArgNum, int maxArgNum);
  Svalue createContinuation(int s);
  Svalue box(Svalue x);

  int push(Svalue x, int s);
  Svalue index(int s, int i) const;
  void indexSet(int s, int i, Svalue v);
  void expandStack();
  int shiftArgs(int n, int m, int s);
  int modifyRestParams(int argNum, int minArgNum, int s);
  Svalue createRestParams(int argNum, int minArgNum, int s);
  void unshiftArgs(int argNum, int s);

  Svalue saveStack(int s);
  int restoreStack(Svalue v);

  int pushArgs(int argNum, const Svalue* args, int s);

  State* state_;
  Svalue* stack_;
  int stackSize_;

  // Symbols
  Svalue* opcodes_;

  // Global variables
  SHashTable* globalVariableTable_;

  // For native function call.
  int stackPointer_;
  int argNum_;
};

}  // namespace yalp

#endif
