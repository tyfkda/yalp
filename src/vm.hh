//=============================================================================
/// Vm - Yet Another List Processor VM.
//=============================================================================

#ifndef _VM_HH_
#define _VM_HH_

#include "yalp.hh"
#include <setjmp.h>
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
  bool run(Svalue code, Svalue* pResult);

  // Gets argument number for current native function.
  int getArgNum() const;
  // Gets argument value for the index.
  Svalue getArg(int index) const;

  Svalue referGlobal(Svalue sym, bool* pExist);
  void defineGlobal(Svalue sym, Svalue value);
  bool assignGlobal(Svalue sym, Svalue value);
  void defineNative(const char* name, NativeFuncType func, int minArgNum) {
    defineNative(name, func, minArgNum, minArgNum);
  }
  void defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);

  bool funcall(Svalue fn, int argNum, const Svalue* args, Svalue* pResult);
  Svalue tailcall(Svalue fn, int argNum, const Svalue* args);
  void resetError();

  int getCallStackDepth() const  { return callStack_.size(); }
  const CallStack* getCallStack() const  { return &callStack_[0]; }

  jmp_buf* setJmpbuf(jmp_buf* jmp);
  void longJmp();

  void markRoot();

  void reportDebugInfo() const;

private:
  Vm(State* state);
  ~Vm();
  void installNativeFunctions();
  Svalue runLoop();
  int findOpcode(Svalue op);
  Svalue createClosure(Svalue body, int nfree, int s, int minArgNum, int maxArgNum);
  Svalue createContinuation(int s, Svalue tail);
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
  Svalue funcallExec(Svalue fn, int argNum, const Svalue* args);

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

  Svalue endOfCode_;
  Svalue return_;

  Svalue a_;  // Accumulator.
  Svalue x_;  // Running code.
  int f_;     // Frame pointer.
  Svalue c_;  // Current closure.
  int s_;     // Stack pointer.

  jmp_buf* jmp_;

  std::vector<CallStack> callStack_;
};

}  // namespace yalp

#endif
