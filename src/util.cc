//=============================================================================
/// Utility functions
//=============================================================================

#include "build_env.hh"
#include "yalp/util.hh"
#include "yalp.hh"
#include "yalp/object.hh"
#include "yalp/stream.hh"
#include "allocator.hh"

namespace yalp {

Value car(Value s) {
  return s.getType() == TT_CELL ?
    static_cast<Cell*>(s.toObject())->car() : s;
}

Value cdr(Value s) {
  return s.getType() == TT_CELL ?
    static_cast<Cell*>(s.toObject())->cdr() : Value::NIL;
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

Value listToVector(State* state, Value ls) {
  int len = length(ls);
  Allocator* allocator = state->getAllocator();
  Vector* vector = allocator->newObject<Vector>(allocator, len);
  Value p = ls;
  for (int i = 0; i < len; ++i) {
    Cell* q = static_cast<Cell*>(p.toObject());
    vector->set(i, q->car());
    p = q->cdr();
  }
  return Value(vector);
}

struct FormatParams {
  virtual bool valueOnly()  { return true; }
  virtual const Value* getValue() = 0;
  virtual const char* getString()  { return NULL; }
  virtual int getInt()  { return 0; }
};

struct ValistParams : public FormatParams {
  ValistParams(va_list ap)  { va_copy(ap_, ap); }
  virtual bool valueOnly()  { return false; }
  virtual const Value* getValue() override  { return va_arg(ap_, const Value*); }
  virtual const char* getString() override  { return va_arg(ap_, const char*); }
  virtual int getInt() override  { return va_arg(ap_, int); }
private:
  va_list ap_;
};

struct ValueParams : public FormatParams {
  ValueParams(const Value* values) : values_(values)  {}
  virtual const Value* getValue() override  { return values_++; }
private:
  const Value* values_;
};

static void format(State* state, Stream* out, const char* fmt, FormatParams* params) {
  const char* prev = fmt;
  for (const char* p = fmt; *p != '\0'; ++p) {
    if (*p != '%')
      continue;

    if (prev != p)
      out->write(prev, p - prev);

    switch (*(++p)) {
    L_default:
    default:
      out->write(*reinterpret_cast<const unsigned char*>(p));
      break;
    case '\0':  goto L_exit;
    case '@':
      {
        const Value* p = params->getValue();
        p->output(state, out, false);
      }
      break;
    case 's':
      if (params->valueOnly())
        goto L_default;
      {
        const char* p = params->getString();
        out->write(p);
      }
      break;
    case 'd':
      if (params->valueOnly())
        goto L_default;
      {
        int x = params->getInt();
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

void format(State* state, Stream* out, const char* fmt, va_list ap) {
  ValistParams params(ap);
  format(state, out, fmt, &params);
}

void format(State* state, Stream* out, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  format(state, out, fmt, ap);
  va_end(ap);
}

void format(State* state, Stream* out, const char* fmt, const Value* values) {
  ValueParams params(values);
  format(state, out, fmt, &params);
}

}  // namespace yalp
