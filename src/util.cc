//=============================================================================
/// Utility functions
//=============================================================================

#include "yalp/util.hh"
#include "yalp.hh"
#include "yalp/object.hh"

namespace yalp {

Svalue list(State* state, Svalue v1) {
  return state->cons(v1, state->nil());
}

Svalue list(State* state, Svalue v1, Svalue v2) {
  return state->cons(v1, state->cons(v2, state->nil()));
}

Svalue list(State* state, Svalue v1, Svalue v2, Svalue v3) {
  return state->cons(v1, state->cons(v2, state->cons(v3, state->nil())));
}

Svalue nreverse(State* state, Svalue v) {
  if (v.getType() != TT_CELL)
    return v;

  Svalue tail = state->nil();
  for (;;) {
    Cell* cell = static_cast<Cell*>(v.toObject());
    Svalue d = cell->cdr();
    cell->setCdr(tail);
    if (d.getType() != TT_CELL)
      return v;
    tail = v;
    v = d;
  }
}

int length(State* state, Svalue v) {
  int len = 0;
  for (; v.getType() == TT_CELL; v = state->cdr(v))
    ++len;
  return len;
}

}  // namespace yalp
