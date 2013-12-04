//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#ifndef _VM_HH_
#define _VM_HH_

#include "yalp.hh"
#include <vector>

namespace yalp {

class Callable;
class SHashTable;
class Symbol;

// Vm class.
class Vm {
public:
  struct CallStack {
    Callable* callable;
    bool isTailCall;
  };

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

  int getCallStackDepth() const  { return callStack_.size(); }
  const CallStack* getCallStack() const  { return &callStack_[0]; }

  void markRoot();

  void reportDebugInfo() const;

private:
  Vm(State* state);
  ~Vm();
  void installNativeFunctions();
  Svalue runLoop();
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

  void pushCallStack(Callable* callable);
  void popCallStack();
  void shiftCallStack();

  State* state_;
  Svalue* stack_;
  int stackSize_;

  // Symbols
  Svalue* opcodes_;

  // Global variables
  SHashTable* globalVariableTable_;

  Svalue return_;

  Svalue a_;  // Accumulator.
  Svalue x_;  // Running code.
  int f_;     // Frame pointer.
  Svalue c_;  // Current closure.
  int s_;     // Stack pointer.

  std::vector<CallStack> callStack_;
};

}  // namespace yalp

#endif
