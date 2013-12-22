// This file is generated from tool/gen_template.rb
#define ARG(i)  Type<P##i>::get(state, state->getArg(i))
#define CHECK(i)  {if (!Type<P##i>::check(state->getArg(i))) return RAISE(i);}
#define RAISE(i)  raiseTypeError(state, i, Type<P##i>::TYPE_NAME, state->getArg(i))

// Template class for Selector.
// Selector template class is specialized with type.
template <class T>
struct Selector {
  // Template specialization.
  //static Value call(State* state) = 0;
};

// void f(void);
template<>
struct Selector<void (*)(void)> {
  typedef void (*FuncType)(void);
  static const int NPARAM = 0;
  static Value call(State* state) {
    (void)(state);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)();
    return Value::NIL;
  }
};

// R f(void);
template<class R>
struct Selector<R (*)(void)> {
  typedef R (*FuncType)(void);
  static const int NPARAM = 0;
  static Value call(State* state) {
    (void)(state);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)();
    return Type<R>::ret(state, result);
  }
};

// void f(P0);
template<class P0>
struct Selector<void (*)(P0)> {
  typedef void (*FuncType)(P0);
  static const int NPARAM = 1;
  static Value call(State* state) {
    CHECK(0);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0));
    return Value::NIL;
  }
};

// R f(P0);
template<class R, class P0>
struct Selector<R (*)(P0)> {
  typedef R (*FuncType)(P0);
  static const int NPARAM = 1;
  static Value call(State* state) {
    CHECK(0);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1);
template<class P0, class P1>
struct Selector<void (*)(P0, P1)> {
  typedef void (*FuncType)(P0, P1);
  static const int NPARAM = 2;
  static Value call(State* state) {
    CHECK(0); CHECK(1);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1));
    return Value::NIL;
  }
};

// R f(P0, P1);
template<class R, class P0, class P1>
struct Selector<R (*)(P0, P1)> {
  typedef R (*FuncType)(P0, P1);
  static const int NPARAM = 2;
  static Value call(State* state) {
    CHECK(0); CHECK(1);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2);
template<class P0, class P1, class P2>
struct Selector<void (*)(P0, P1, P2)> {
  typedef void (*FuncType)(P0, P1, P2);
  static const int NPARAM = 3;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2));
    return Value::NIL;
  }
};

// R f(P0, P1, P2);
template<class R, class P0, class P1, class P2>
struct Selector<R (*)(P0, P1, P2)> {
  typedef R (*FuncType)(P0, P1, P2);
  static const int NPARAM = 3;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3);
template<class P0, class P1, class P2, class P3>
struct Selector<void (*)(P0, P1, P2, P3)> {
  typedef void (*FuncType)(P0, P1, P2, P3);
  static const int NPARAM = 4;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3));
    return Value::NIL;
  }
};

// R f(P0, P1, P2, P3);
template<class R, class P0, class P1, class P2, class P3>
struct Selector<R (*)(P0, P1, P2, P3)> {
  typedef R (*FuncType)(P0, P1, P2, P3);
  static const int NPARAM = 4;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3, P4);
template<class P0, class P1, class P2, class P3, class P4>
struct Selector<void (*)(P0, P1, P2, P3, P4)> {
  typedef void (*FuncType)(P0, P1, P2, P3, P4);
  static const int NPARAM = 5;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
    return Value::NIL;
  }
};

// R f(P0, P1, P2, P3, P4);
template<class R, class P0, class P1, class P2, class P3, class P4>
struct Selector<R (*)(P0, P1, P2, P3, P4)> {
  typedef R (*FuncType)(P0, P1, P2, P3, P4);
  static const int NPARAM = 5;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3, P4, P5);
template<class P0, class P1, class P2, class P3, class P4, class P5>
struct Selector<void (*)(P0, P1, P2, P3, P4, P5)> {
  typedef void (*FuncType)(P0, P1, P2, P3, P4, P5);
  static const int NPARAM = 6;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
    return Value::NIL;
  }
};

// R f(P0, P1, P2, P3, P4, P5);
template<class R, class P0, class P1, class P2, class P3, class P4, class P5>
struct Selector<R (*)(P0, P1, P2, P3, P4, P5)> {
  typedef R (*FuncType)(P0, P1, P2, P3, P4, P5);
  static const int NPARAM = 6;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3, P4, P5, P6);
template<class P0, class P1, class P2, class P3, class P4, class P5, class P6>
struct Selector<void (*)(P0, P1, P2, P3, P4, P5, P6)> {
  typedef void (*FuncType)(P0, P1, P2, P3, P4, P5, P6);
  static const int NPARAM = 7;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5); CHECK(6);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6));
    return Value::NIL;
  }
};

// R f(P0, P1, P2, P3, P4, P5, P6);
template<class R, class P0, class P1, class P2, class P3, class P4, class P5, class P6>
struct Selector<R (*)(P0, P1, P2, P3, P4, P5, P6)> {
  typedef R (*FuncType)(P0, P1, P2, P3, P4, P5, P6);
  static const int NPARAM = 7;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5); CHECK(6);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3, P4, P5, P6, P7);
template<class P0, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
struct Selector<void (*)(P0, P1, P2, P3, P4, P5, P6, P7)> {
  typedef void (*FuncType)(P0, P1, P2, P3, P4, P5, P6, P7);
  static const int NPARAM = 8;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5); CHECK(6); CHECK(7);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7));
    return Value::NIL;
  }
};

// R f(P0, P1, P2, P3, P4, P5, P6, P7);
template<class R, class P0, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
struct Selector<R (*)(P0, P1, P2, P3, P4, P5, P6, P7)> {
  typedef R (*FuncType)(P0, P1, P2, P3, P4, P5, P6, P7);
  static const int NPARAM = 8;
  static Value call(State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5); CHECK(6); CHECK(7);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7));
    return Type<R>::ret(state, result);
  }
};

#undef ARG
#undef CHECK
