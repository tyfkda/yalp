#!/bin/zsh

################################################################
# Test framework.

function error_exit() {
  echo -n -e '\e[1;31m[ERROR]\e[0m '
  echo "$1"
  exit 1
}

function run() {
  echo -n "Testing $1 ... "
  result=$(echo "(write ((^() $3)))" | gosh compiler.scm)
  if [ "$result" != "$2" ]; then
    error_exit "$2 expected, but got '$result'"
  fi
  echo ok
}

function run_raw() {
  echo -n "Testing $1 ... "
  result=$(echo "$3" | gosh compiler.scm)
  if [ "$result" != "$2" ]; then
    error_exit "$2 expected, but got '$result'"
  fi
  echo ok
}

function fail() {
  echo -n "Testing $1 ... "
  echo "$2" | gosh compiler.scm 2>& /dev/null
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
run set-local 111 '((^(x)
                      (set! x 111)
                      x)
                    123)'
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
run global-var 111 '(def global 111)
                    global'
run restargs '(1 (2 3))' '((^(x . y) (list x y)) 1 2 3)'
run restargs-all '(1 2 3)' '((^ x x) 1 2 3)'
run empty-body nil '((^ ()))'

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

# Test native functions
run cons '(1 . 2)' '(cons 1 2)'
run car '1' "(car '(1 2 3))"
run cdr '(2 3)' "(cdr '(1 2 3))"
run set-car! '(3 2)' "((^(x) (set-car! x 3) x) '(1 2))"
run set-cdr! '(1 . 3)' "((^(x) (set-cdr! x 3) x) '(1 2))"
run list '(1 2 (3 4))' "(list 1 2 '(3 4))"
run 'list*' '(1 2 3 4)' "(list* 1 2 '(3 4))"
run consp 't' "(consp '(1 2))"
run consp2 'nil' "(consp 'symbol)"
run symbolp 't' "(symbolp 'symbol)"
run symbolp2 'nil' "(symbolp '(1 2))"
run symbolp-nil 't' "(symbolp nil)"
run append '(1 2 3 4 5 6)' "(append '(1 2) '(3 4) '(5 6))"
run + '15' '(+ 1 2 3 4 5)'
run - '7' '(- 10 3)'
run negate '-10' '(- 10)'
run '*' '120' '(* 1 2 3 4 5)'
run / '3' '(/ 10 3)'
run +float '1.23' '(+ 1 0.23)'
run -float '0.77' '(- 1 0.23)'
run -negate '-0.23' '(- 0.23)'
run '*float' '0.46' '(* 2 0.23)'
run /float '8.695652173913043' '(/ 2 0.23)'
run /invert '4.3478260869565215' '(/ 0.23)'

run is t '(is 123 123)'
run iso-list t "(iso '(1 2 3) '(1 2 3))"
run iso-string t '(iso "string" "string")'
run '<' t '(< 1 2)'
run '<' nil '(< 2 2)'
run '>' t '(> 2 1)'
run '<=' t '(<= 2 2)'
run '>=' t '(>= 2 2)'

run apply-native 15 "(apply + 1 2 '(3 4 5))"
run apply-compound 15 "(apply (^(a b c d e) (+ a b c d e)) 1 2 '(3 4 5))"

# Hash table
run hash-table 123 "((^(h)
                        (hash-table-put! h 'key 123)
                        (hash-table-get h 'key))
                     (make-hash-table))"

# Scheme - yalp value differences
run '() is false' 3 '(if () 2 3)'
run '() is nil' t '(is () nil)'

# Fail cases
fail unbound 'abc'
fail no-global '((^(x) y) 123)'
fail invalid-apply '(1 2 3)'
fail too-few-arg-native '(cons 1)'
fail too-many-arg-native '(cons 1 2 3)'
fail too-few-arg-lambda '((^(x y)) 1)'
fail too-many-arg-lambda '((^(x y)) 1 2 3)'
fail empty-param-not-rest-param '((^() nil) 1 2 3)'

################################################################
# All tests succeeded.

echo -n -e "\e[1;32mALL SUCCESS!\e[0m\n"
