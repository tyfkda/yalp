//=============================================================================
/// Utility functions
//=============================================================================

#include "yalp/util.hh"
#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"

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

void format(State* state, Stream* out, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  format(state, out, fmt, ap);
  va_end(ap);
}

void format(State* state, Stream* out, const char* fmt, va_list ap) {
  const char* prev = fmt;
  for (const char* p = fmt; *p != '\0'; ++p) {
    if (*p != '%')
      continue;

    if (prev != p)
      out->write(prev, p - prev);

    switch (*(++p)) {
    default:
      out->write(*reinterpret_cast<const unsigned char*>(p));
      break;
    case '\0':  goto L_exit;
    case '@':
      {
        const Svalue* p = va_arg(ap, const Svalue*);
        p->output(state, out, false);
      }
      break;
    case 's':
      {
        const char* p = va_arg(ap, const char*);
        out->write(p);
      }
      break;
    case 'd':
      {
        int x = va_arg(ap, int);
        char buffer[sizeof(int) * 3 + 1];
        snprintf(buffer, sizeof(buffer), "%d", x);
        out->write(buffer);
      }
      break;
    }
    prev = p + 1;
  }
 L_exit:
  if (*prev != '\0')
    out->write(prev);
}

}  // namespace yalp
