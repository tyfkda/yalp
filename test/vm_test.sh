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
  result=$(cd .. && echo "(write ((^() $3)))" | ./yalp)
  if [ "$result" != "$2" ]; then
    error_exit "$2 expected, but got '$result'"
  fi
  echo ok
}

function fail() {
  echo -n "Testing $1 ... "
  cd .. && echo "$2" | ./yalp 2>& /dev/null
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
run global-var 111 '((^()
                       ((^()
                          (set! global 111)))
                       global))'
run restargs '(1 (2 3))' '((^(x . y) (list x y)) 1 2 3)'
run restargs-all '(1 2 3)' '((^ x x) 1 2 3)'
run empty-body nil '((^ ()))'

# Test native functions
run cons '(1 . 2)' '(cons 1 2)'
run car '1' "(car '(1 2 3))"
run cdr '(2 3)' "(cdr '(1 2 3))"
run rplaca '(3 2)' "((^(x) (rplaca x 3) x) '(1 2))"
run rplacd '(1 . 3)' "((^(x) (rplacd x 3) x) '(1 2))"
run list '(1 2 (3 4))' "(list 1 2 '(3 4))"
run 'list*' '(1 2 3 4)' "(list* 1 2 '(3 4))"
run consp 't' "(consp '(1 2))"
run consp2 'nil' "(consp 'symbol)"
run symbolp 't' "(symbolp 'symbol)"
run symbolp2 'nil' "(symbolp '(1 2))"
run append '(1 2 3 4 5 6)' "(append '(1 2) '(3 4) '(5 6))"
run + '15' '(+ 1 2 3 4 5)'
run - '7' '(- 10 3)'
run negate '-10' '(- 10)'
run '*' '120' '(* 1 2 3 4 5)'
run / '3' '(/ 10 3)'

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

# Fail cases
fail unbound 'abc'
fail no-global '((^(x) y) 123)'
fail invalid-apply '(1 2 3)'
fail too-few-arg-native '(cons 1)'
fail too-many-arg-native '(cons 1 2 3)'
fail too-few-arg-lambda '((^(x y)) 1)'
fail too-many-arg-lambda '((^(x y)) 1 2 3)'

################################################################
# All tests succeeded.

echo -n -e "\e[1;32mALL SUCCESS!\e[0m\n"
