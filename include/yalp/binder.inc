// This file is generated from tool/gen_template.rb
#define ARG(i)  Type<P##i>::get(state, state->getArg(i))
#define CHECK(i)  {if (!Type<P##i>::check(state->getArg(i))) return RAISE(i);}
#define RAISE(i)  raiseTypeError(state, i, Type<P##i>::TYPE_NAME, state->getArg(i))

// Template class for Binder.
// Binder template class is specialized with type.
template <class T>
struct Binder {
  // Template specialization.
  //static Value call(State* state) = 0;
};

// void f(void);
template<>
struct Binder<void (*)(void)> {
  typedef void (*FuncType)(void);
  static const int NPARAM = 0;
  static yalp::Value call(yalp::State* state) {
    (void)(state);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)();
    return yalp::Value::NIL;
  }
};

// R f(void);
template<class R>
struct Binder<R (*)(void)> {
  typedef R (*FuncType)(void);
  static const int NPARAM = 0;
  static yalp::Value call(yalp::State* state) {
    (void)(state);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)();
    return Type<R>::ret(state, result);
  }
};

// void f(P0);
template<class P0>
struct Binder<void (*)(P0)> {
  typedef void (*FuncType)(P0);
  static const int NPARAM = 1;
  static yalp::Value call(yalp::State* state) {
    CHECK(0);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0));
    return yalp::Value::NIL;
  }
};

// R f(P0);
template<class R, class P0>
struct Binder<R (*)(P0)> {
  typedef R (*FuncType)(P0);
  static const int NPARAM = 1;
  static yalp::Value call(yalp::State* state) {
    CHECK(0);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1);
template<class P0, class P1>
struct Binder<void (*)(P0, P1)> {
  typedef void (*FuncType)(P0, P1);
  static const int NPARAM = 2;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1));
    return yalp::Value::NIL;
  }
};

// R f(P0, P1);
template<class R, class P0, class P1>
struct Binder<R (*)(P0, P1)> {
  typedef R (*FuncType)(P0, P1);
  static const int NPARAM = 2;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2);
template<class P0, class P1, class P2>
struct Binder<void (*)(P0, P1, P2)> {
  typedef void (*FuncType)(P0, P1, P2);
  static const int NPARAM = 3;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2));
    return yalp::Value::NIL;
  }
};

// R f(P0, P1, P2);
template<class R, class P0, class P1, class P2>
struct Binder<R (*)(P0, P1, P2)> {
  typedef R (*FuncType)(P0, P1, P2);
  static const int NPARAM = 3;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3);
template<class P0, class P1, class P2, class P3>
struct Binder<void (*)(P0, P1, P2, P3)> {
  typedef void (*FuncType)(P0, P1, P2, P3);
  static const int NPARAM = 4;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3));
    return yalp::Value::NIL;
  }
};

// R f(P0, P1, P2, P3);
template<class R, class P0, class P1, class P2, class P3>
struct Binder<R (*)(P0, P1, P2, P3)> {
  typedef R (*FuncType)(P0, P1, P2, P3);
  static const int NPARAM = 4;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3, P4);
template<class P0, class P1, class P2, class P3, class P4>
struct Binder<void (*)(P0, P1, P2, P3, P4)> {
  typedef void (*FuncType)(P0, P1, P2, P3, P4);
  static const int NPARAM = 5;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
    return yalp::Value::NIL;
  }
};

// R f(P0, P1, P2, P3, P4);
template<class R, class P0, class P1, class P2, class P3, class P4>
struct Binder<R (*)(P0, P1, P2, P3, P4)> {
  typedef R (*FuncType)(P0, P1, P2, P3, P4);
  static const int NPARAM = 5;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3, P4, P5);
template<class P0, class P1, class P2, class P3, class P4, class P5>
struct Binder<void (*)(P0, P1, P2, P3, P4, P5)> {
  typedef void (*FuncType)(P0, P1, P2, P3, P4, P5);
  static const int NPARAM = 6;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
    return yalp::Value::NIL;
  }
};

// R f(P0, P1, P2, P3, P4, P5);
template<class R, class P0, class P1, class P2, class P3, class P4, class P5>
struct Binder<R (*)(P0, P1, P2, P3, P4, P5)> {
  typedef R (*FuncType)(P0, P1, P2, P3, P4, P5);
  static const int NPARAM = 6;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3, P4, P5, P6);
template<class P0, class P1, class P2, class P3, class P4, class P5, class P6>
struct Binder<void (*)(P0, P1, P2, P3, P4, P5, P6)> {
  typedef void (*FuncType)(P0, P1, P2, P3, P4, P5, P6);
  static const int NPARAM = 7;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5); CHECK(6);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6));
    return yalp::Value::NIL;
  }
};

// R f(P0, P1, P2, P3, P4, P5, P6);
template<class R, class P0, class P1, class P2, class P3, class P4, class P5, class P6>
struct Binder<R (*)(P0, P1, P2, P3, P4, P5, P6)> {
  typedef R (*FuncType)(P0, P1, P2, P3, P4, P5, P6);
  static const int NPARAM = 7;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5); CHECK(6);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6));
    return Type<R>::ret(state, result);
  }
};

// void f(P0, P1, P2, P3, P4, P5, P6, P7);
template<class P0, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
struct Binder<void (*)(P0, P1, P2, P3, P4, P5, P6, P7)> {
  typedef void (*FuncType)(P0, P1, P2, P3, P4, P5, P6, P7);
  static const int NPARAM = 8;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5); CHECK(6); CHECK(7);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7));
    return yalp::Value::NIL;
  }
};

// R f(P0, P1, P2, P3, P4, P5, P6, P7);
template<class R, class P0, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
struct Binder<R (*)(P0, P1, P2, P3, P4, P5, P6, P7)> {
  typedef R (*FuncType)(P0, P1, P2, P3, P4, P5, P6, P7);
  static const int NPARAM = 8;
  static yalp::Value call(yalp::State* state) {
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(5); CHECK(6); CHECK(7);
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7));
    return Type<R>::ret(state, result);
  }
};

#undef ARG
#undef CHECK
