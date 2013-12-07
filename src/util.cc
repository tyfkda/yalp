//=============================================================================
/// Utility functions
//=============================================================================

#include "yalp/util.hh"
#include "yalp.hh"
#include "yalp/object.hh"

namespace yalp {

Svalue list(State* state, Svalue v1) {
  return state->cons(v1, Svalue::NIL);
}

Svalue list(State* state, Svalue v1, Svalue v2) {
  return state->cons(v1, state->cons(v2, Svalue::NIL));
}

Svalue list(State* state, Svalue v1, Svalue v2, Svalue v3) {
  return state->cons(v1, state->cons(v2, state->cons(v3, Svalue::NIL)));
}

Svalue nreverse(Svalue v) {
  if (v.getType() != TT_CELL)
    return v;

  Svalue tail = Svalue::NIL;
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

int length(Svalue v) {
  int len = 0;
  for (; v.getType() == TT_CELL; v = static_cast<Cell*>(v.toObject())->cdr())
    ++len;
  return len;
}

}  // namespace yalp
