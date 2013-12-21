//=============================================================================
/// Utility functions
//=============================================================================

#include "yalp/util.hh"
#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"

namespace yalp {

unsigned int strHash(const char* s) {
  unsigned int v = 0;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
       *p != '\0'; ++p)
    v = v * 17 + 1 + *p;
  return v;
}

Value list(State* state, Value v1) {
  return state->cons(v1, Value::NIL);
}

Value list(State* state, Value v1, Value v2) {
  return state->cons(v1, state->cons(v2, Value::NIL));
}

Value list(State* state, Value v1, Value v2, Value v3) {
  return state->cons(v1, state->cons(v2, state->cons(v3, Value::NIL)));
}

Value nreverse(Value v) {
  if (v.getType() != TT_CELL)
    return v;

  Value tail = Value::NIL;
  for (;;) {
    Cell* cell = static_cast<Cell*>(v.toObject());
    Value d = cell->cdr();
    cell->setCdr(tail);
    if (d.getType() != TT_CELL)
      return v;
    tail = v;
    v = d;
  }
}

int length(Value v) {
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
        const Value* p = va_arg(ap, const Value*);
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
