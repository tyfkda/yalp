#!/bin/bash

################################################################
# Test framework.

function error_exit() {
  echo -n -e "\033[1;31m[ERROR]\033[0;39m "
  echo "$1"
  exit 1
}

function run() {
  echo -n "Testing $1 ... "
  result=$(echo "(write ((^() $3)))" | ../yalp -L ../boot.bin)
  if [ "$result" != "$2" ]; then
    error_exit "$2 expected, but got '$result'"
  fi
  echo ok
}

function run_raw() {
  echo -n "Testing $1 ... "
  result=$(echo "$3" | ../yalp -L ../boot.bin)
  if [ "$result" != "$2" ]; then
    error_exit "$2 expected, but got '$result'"
  fi
  echo ok
}

function fail() {
  echo -n "Testing $1 ... "
  echo "$2" | ../yalp -L ../boot.bin 1>/dev/null 2>/dev/null
  if [ $? -eq 0 ]; then
    error_exit "Failure expected, but succeeded!"
  fi
  echo ok
}

################################################################
# Test cases.

run integer 123 '123'
run nil nil 'nil'
run t t 't'
run quote abc '(quote abc)'
run string '"string"' '"string"'
run float '1.230000' '1.23'
run if-true 2 '(if 1 2 3)'
run if-false 3 '(if nil 2 3)'
run no-else 2 '(if 1 2)'
run no-else2 nil '(if nil 2)'
run lambda-invoke 123 '((^(x) x) 123)'
run multiple-exp 3 '((^()
                       1
                       2
                       3)
                     )'
run set-local 2 '((^(x)
                      (set! x 2)
                      x)
                    1)'
run set-result 2 '((^(x)
                      (set! x 2))
                    1)'
run multi-set '(3 4)' '((^(x y)
                          (set! x 3
                                y 4)
                          (list x y))
                        1 2)'
run closure 1 '(((^(x)
                   (^(y)
                     x))
                 1)
                2)'
run closure-set 23 '((^(x)
                       ((^()
                          (set! x 23)))
                       x)
                     1)'
run call/cc 123 '(call/cc
                   (^(cc)
                     (cc 123)))'
run call/cc-nil nil '(call/cc
                       (^(cc)
                         (cc)))'
run_raw invoke-call/cc '1234' '(def *cc* ())
                               (display (call/cc (^(cc) (set! *cc* cc) 12)))
                               (*cc* 34)'
run call/cc-indirect 123 '((^(f)
                             (call/cc f))
                           (^(cc) (cc 123)))'
run global-var 111 '(def global 111)
                    global'
run restargs-direct '(1 (2 3))' '((^(x &rest y) (list x y)) 1 2 3)'
run restargs '(1 (2 3))' '((^(f) (f 1 2 3)) (^ (x &rest y) (list x y)))'
run restargs-all-direct '(1 2 3)' '((^ (&rest x) x) 1 2 3)'
run restargs-all '(1 2 3)' '((^(f) (f 1 2 3)) (^ (&rest x) x))'
run empty-body nil '((^ ()))'

# Multiple values
run values 1 '(values 1 2 3)'
run values-can-use-in-expression 14 '(+ 1 (values 10 20 30) 3)'
run receive '(1 2 3)' '(receive (x y z) (values 1 2 3) (list x y z))'
run nested-values '(1 9 3)' '(receive (x y z) (values 1 (values 9 8 7) 3) (list x y z))'
run empty-values nil '((^(x) (values)) 123)'
run receive-rest-params '(1 (2 3))' '(receive (x &rest y) (values 1 2 3) (list x y))'
run receive-empty-rest '(1 nil)' '(receive (x &rest y) (values 1) (list x y))'
run receive-normal '((1 2 3))' '(receive (&rest all) (list 1 2 3) all)'
run receive-empty 'nil' '(receive (&rest all) (values) all)'

# Abbreviated form
run quote-x "'x" "''x"
run quasiquote-x "\`x" "'\`x"
run unquote-x ",x" "',x"
run unquote-splicing-x ",@x" "',@x"

# Macro
run_raw nil! nil "(defmacro nil! (sym)
                    (list 'def sym nil))
                  (nil! xyz)
                  (write xyz)"
run_raw hide-macro bar "(defmacro foo(x) \`'(foo ,x))
                        ((^(foo) (print (foo))) (^() 'bar))"
run_raw macro-capture baz "((^(bar) (defmacro foo() (list 'quote bar))) 'baz) (print (foo))"

# Test native functions
run cons '(1 . 2)' '(cons 1 2)'
run car '1' "(car '(1 2 3))"
run cdr '(2 3)' "(cdr '(1 2 3))"
run car-no-cell 123 "(car 123)"
run cdr-no-cell nil "(cdr 123)"
run set-car! '(3 2)' "((^(x) (set-car! x 3) x) '(1 2))"
run set-cdr! '(1 . 3)' "((^(x) (set-cdr! x 3) x) '(1 2))"
run list '(1 2 (3 4))' "(list 1 2 '(3 4))"
run 'list*' '(1 2 3 4)' "(list* 1 2 '(3 4))"
run 'pair?' 't' "(pair? '(1 2))"
run 'pair?2' 'nil' "(pair? 'symbol)"
run 'symbol?' 't' "(symbol? 'symbol)"
run 'symbol?2' 'nil' "(symbol? '(1 2))"
run 'symbol?-nil' 't' "(symbol? nil)"
run append '(1 2 3 4 5 6)' "(append '(1 2) '(3 4) '(5 6))"
run + '15' '(+ 1 2 3 4 5)'
run - '7' '(- 10 3)'
run negate '-10' '(- 10)'
run '*' '120' '(* 1 2 3 4 5)'
run / '3' '(/ 10 3)'

run 'eq?' t '(eq? 123 123)'
run 'equal?-list' t "(equal? '(1 2 3) '(1 2 3))"
run 'equal?-string' t '(equal? "string" "string")'
run '<' t '(< 1 2)'
run '<' nil '(< 2 2)'
run '>' t '(> 2 1)'
run '<=' t '(<= 2 2)'
run '>=' t '(>= 2 2)'

# Float
run 'float-eq?' nil '(eq? 1.0 1.0)'
run 'float-equal?' t '(equal? 1.0 1.0)'
run +float '1.230000' '(+ 1 0.23)'
run -float '0.770000' '(- 1 0.23)'
run -negate '-0.230000' '(- 0.23)'
run '*float' '0.460000' '(* 2 0.23)'
run /float '8.695652' '(/ 2 0.23)'
run /invert '4.347826' '(/ 0.23)'
run '<float' t '(< 1 1.1)'

run apply-native 15 "(apply + 1 2 '(3 4 5))"
run apply-compound 15 "(apply (^(a b c d e) (+ a b c d e)) 1 2 '(3 4 5))"

# Hash table
run hash-table 123 "((^(h)
                        (table-put! h 'key 123)
                        (table-get h 'key))
                     (table))"

# eval
run eval "'x" "(eval '(quote (quote x)))"
run eval "x" "(eval (eval '(quote (quote x))))"

# Scheme - yalp value differences
run '() is false' 3 '(if () 2 3)'
run '() is nil' t '(eq? () nil)'

# Fail cases
fail unbound 'abc'
fail no-global '((^(x) y) 123)'
fail invalid-apply '(1 2 3)'
fail too-few-arg-native '(cons 1)'
fail too-many-arg-native '(cons 1 2 3)'
fail too-few-arg-lambda-direct '((^(x y)) 1)'
fail too-few-arg-lambda '((^(f) (f 1)) (^(x y)))'
fail too-many-arg-lambda-direct '((^(x y)) 1 2 3)'
fail too-many-arg-lambda '((^(f) (f 1 2 3)) (^(x y)))'
fail too-few-arg-receive '(receive (x y) (values 1))'
fail too-many-arg-receive '(receive (x y) (values 1 2 3))'
fail empty-param-not-rest-param-direct '((^() nil) 1 2 3)'
fail empty-param-not-rest-param '((^(f) (f 1 2 3)) (^() nil))'
fail set-unbound-var '(set! x 123)'

################################################################
# All tests succeeded.

echo -n -e "\033[1;32mVM-TEST ALL SUCCEEDED!\033[0;39m\n"
