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
  Value run(Value code);

  inline Sobject* getFunc() const;
  // Gets argument number for current native function.
  inline int getArgNum() const;
  // Gets argument value for the index.
  inline Value getArg(int index) const;

  inline Value multiValues();
  inline Value multiValues(Value v0);
  inline Value multiValues(Value v0, Value v1);
  inline Value multiValues(Value v0, Value v1, Value v2);

  // Gets result number.
  inline int getResultNum() const;
  // Gets result value for the index.
  inline Value getResult(int index) const;

  Value referGlobal(Value sym, bool* pExist);
  void defineGlobal(Value sym, Value value);
  bool assignGlobal(Value sym, Value value);
  void defineNative(const char* name, NativeFuncType func, int minArgNum, int maxArgNum);
  Value getMacro(Value name);
  void defineMacro(Value name, Value func);

  // Calls function.
  Value funcall(Value fn, int argNum, const Value* args);
  // Calls function at tail position.
  inline Value tailcall(Value fn, int argNum, const Value* args);

  void resetError();

  inline int getCallStackDepth() const;
  inline const CallStack* getCallStack() const;

  void setTrace(bool b);

  void markRoot();

  void reportDebugInfo() const;

private:
  Vm(State* state);
  ~Vm();
  void installNativeFunctions();
  Value runLoop();
  Value createClosure(Value body, int nfree, int s, int minArgNum, int maxArgNum);
  Value createContinuation(int s);
  Value funcallSetup(Value fn, int argNum, const Value* args, bool tailcall);
  void apply(Value fn, int argNum);

  void reserveStack(int n);  // Ensure the stack has enough size of n
  int modifyRestParams(int argNum, int minArgNum, int s);
  Value createRestParams(int argNum, int minArgNum, int s);
  void expandFrame(int n);
  void shrinkFrame(int n);
  void reserveValuesBuffer(int n);
  void storeValues(int n, int s);  // Move arguments from stack to values buffer.
  void restoreValues(int min, int max);
  int pushArgs(int argNum, const Value* args, int s);

  void pushCallStack(Callable* callable);
  void popCallStack();
  void shiftCallStack();

  inline Value index(int s, int i) const;
  inline void indexSet(int s, int i, Value v);
  inline int push(Value x, int s);
  inline int shiftArgs(int n, int m, int s);
  inline bool isTailCall(Value x) const;
  inline int pushCallFrame(Value ret, int s);
  inline int popCallFrame(int s);
  inline Value box(Value x);

  inline int findOpcode(Value op);
  void replaceOpcodes(Value code);

  State* state_;
  Value* stack_;
  int stackSize_;
  bool trace_;

  Value* values_;
  int valuesSize_;
  int valueCount_;

  // Symbols
  Value* opcodes_;

  // Global variables
  SHashTable* globalVariableTable_;

  // Macro table: symbol => closure
  SHashTable* macroTable_;

  Value endOfCode_;
  Value return_;

  Value a_;  // Accumulator.
  Value x_;  // Running code.
  int f_;     // Frame pointer.
  Value c_;  // Current closure.
  int s_;     // Stack pointer.

  std::vector<CallStack> callStack_;
};

Sobject* Vm::getFunc() const  { return c_.toObject(); }
int Vm::getArgNum() const  { return index(f_, -1).toFixnum(); }
Value Vm::getArg(int index) const  { return this->index(f_, index); }

Value Vm::multiValues()  { valueCount_ = 0; return a_ = Value::NIL; }
Value Vm::multiValues(Value v0)  { reserveValuesBuffer(valueCount_ = 1); return v0; }
Value Vm::multiValues(Value v0, Value v1)  { reserveValuesBuffer(valueCount_ = 2); values_[0] = v1; return v0; }
Value Vm::multiValues(Value v0, Value v1, Value v2)  { reserveValuesBuffer(valueCount_ = 3); values_[0] = v2; values_[1] = v1; return v0; }

int Vm::getResultNum() const  { return valueCount_; }
Value Vm::getResult(int index) const  { return (index == 0) ? a_ : values_[valueCount_ - index - 1]; }

Value Vm::tailcall(Value fn, int argNum, const Value* args) {
  shiftCallStack();
  return funcallSetup(fn, argNum, args, true);
}

int Vm::getCallStackDepth() const  { return callStack_.size(); }
const CallStack* Vm::getCallStack() const  { return &callStack_[0]; }

}  // namespace yalp

#endif
