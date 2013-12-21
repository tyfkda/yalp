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

class CallStack {
public:
  Callable* callable;
  bool isTailCall;
};

// Vm class.
class Vm {
public:
  static Vm* create(State* state);
  void release();

  // Execute compiled code.
  Svalue run(Svalue code);

  // Gets argument number for current native function.
  inline int getArgNum() const;
  // Gets argument value for the index.
  inline Svalue getArg(int index) const;

  // Gets result number.
  inline int getResultNum() const;
  // Gets result value for the index.
  inline Svalue getResult(int index) const;

  Svalue referGlobal(Svalue sym, bool* pExist);
  void defineGlobal(Svalue sym, Svalue value);
  bool assignGlobal(Svalue sym, Svalue value);
  void defineNative(const char* name, NativeFuncType func, int minArgNum) {
    defineNative(name, func, minArgNum, minArgNum);
  }
  void defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);
  Svalue getMacro(Svalue name);

  Svalue funcall(Svalue fn, int argNum, const Svalue* args);
  inline Svalue tailcall(Svalue fn, int argNum, const Svalue* args);
  void resetError();

  int getCallStackDepth() const  { return callStack_.size(); }
  const CallStack* getCallStack() const  { return &callStack_[0]; }

  void setTrace(bool b);

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
  Svalue funcallSetup(Svalue fn, int argNum, const Svalue* args, bool tailcall);
  void apply(Svalue fn, int argNum);

  void reserveStack(int n);  // Ensure the stack has enough size of n
  int modifyRestParams(int argNum, int minArgNum, int s);
  Svalue createRestParams(int argNum, int minArgNum, int s);
  void expandFrame(int n);
  void shrinkFrame(int n);
  void storeValues(int n, int s);
  void restoreValues(int min, int max);

  int pushArgs(int argNum, const Svalue* args, int s);

  void pushCallStack(Callable* callable);
  void popCallStack();
  void shiftCallStack();

  void registerMacro(Svalue name, int minParam, int maxParam, Svalue body);

  inline Svalue index(int s, int i) const;
  inline void indexSet(int s, int i, Svalue v);
  inline int push(Svalue x, int s);
  inline int shiftArgs(int n, int m, int s);
  inline bool isTailCall(Svalue x) const;
  inline int pushCallFrame(Svalue ret, int s);
  inline int popCallFrame(int s);
  inline Svalue box(Svalue x);

  State* state_;
  Svalue* stack_;
  int stackSize_;
  bool trace_;

  Svalue* values_;
  int valuesSize_;
  int valueCount_;

  // Symbols
  Svalue* opcodes_;

  // Global variables
  SHashTable* globalVariableTable_;

  // Macro table: symbol => closure
  SHashTable* macroTable_;

  Svalue endOfCode_;
  Svalue return_;

  Svalue a_;  // Accumulator.
  Svalue x_;  // Running code.
  int f_;     // Frame pointer.
  Svalue c_;  // Current closure.
  int s_;     // Stack pointer.

  std::vector<CallStack> callStack_;
};

int Vm::getArgNum() const  { return index(f_, -1).toFixnum(); }
Svalue Vm::getArg(int index) const  { return this->index(f_, index); }
int Vm::getResultNum() const  { return valueCount_; }
Svalue Vm::getResult(int index) const  { return (index == 0) ? a_ : values_[valueCount_ - index - 1]; }

Svalue Vm::tailcall(Svalue fn, int argNum, const Svalue* args) {
  shiftCallStack();
  return funcallSetup(fn, argNum, args, true);
}

}  // namespace yalp

#endif
