#!/usr/bin/env ruby

MAX_PARAM = 8

HEADER = <<EOD
// This file is generated from tool/gen_template.rb
\#define ARG(i)  Type<P##i>::get(state, state->getArg(i))
\#define CHECK(i)  {if (!Type<P##i>::check(state->getArg(i))) return RAISE(i);}
\#define RAISE(i)  raiseTypeError(state, i, Type<P##i>::TYPE_NAME, state->getArg(i))

// Template class for Binder.
// Binder template class is specialized with type.
template <class T>
struct Binder {
  // Template specialization.
  //static Value call(State* state) = 0;
};

EOD

FOOTER = <<EOD
\#undef ARG
\#undef CHECK
EOD

FUNC_TMPL = <<EOD
// void f(%PARAMS%);
template<%CLASSES0%>
struct Binder<void (*)(%PARAMS%)> {
  typedef void (*FuncType)(%PARAMS%);
  static const int NPARAM = %NPARAM%;
  static yalp::Value call(yalp::State* state) {
    %ASSERTS%
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    (*funcPtr)(%ARGS%);
    return yalp::Value::NIL;
  }
};

// R f(%PARAMS%);
template<class R%CLASSES1%>
struct Binder<R (*)(%PARAMS%)> {
  typedef R (*FuncType)(%PARAMS%);
  static const int NPARAM = %NPARAM%;
  static yalp::Value call(yalp::State* state) {
    %ASSERTS%
    FuncType funcPtr = reinterpret_cast<FuncType>(getBindedFuncPtr(state));
    R result = (*funcPtr)(%ARGS%);
    return Type<R>::ret(state, result);
  }
};

EOD

def embed_template(str, nparam)
  if nparam == 0
    params = 'void'
    args = ''
    classes = ''
    asserts = '(void)(state);'  # Surppress warning.
  else
    params = (0...nparam).map {|i| "P#{i}"}.join(', ')
    args = (0...nparam).map {|i| "ARG(#{i})"}.join(', ')
    classes = (0...nparam).map {|i| "class P#{i}"}.join(', ')
    asserts = (0...nparam).map {|i| "CHECK(#{i});"}.join(' ')
  end

  table = {
    '%PARAMS%' => params,
    '%NPARAM%' => nparam.to_s,
    '%ARGS%' => args,
    '%CLASSES0%' => classes,
    '%CLASSES1%' => classes.empty? ? '' : ', ' + classes,
    '%ASSERTS%' => asserts
  }

  return str.gsub(/(#{table.keys.join('|')})/) {|k| table[k]}
end

print HEADER
(0..MAX_PARAM).each do |nparam|
  print embed_template(FUNC_TMPL, nparam)
end
print FOOTER

#
