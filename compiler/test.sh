#!/bin/zsh

TEST_FILE_NAME='test.arc'

################################################################
# Test framework.

function error_exit() {
  echo -n -e '\e[1;31m[ERROR]\e[0m '
  echo "$1"
  exit 1
}

function run() {
  echo -n "Testing $1 ... "
  result=$(echo "$3" > $TEST_FILE_NAME && gosh compiler.scm $TEST_FILE_NAME)
  if [ "$result" != "$2" ]; then
    error_exit "$2 expected, but got $result"
  fi
  echo ok
}

function fail() {
  echo -n "Testing $1 ... "
  echo "$2" > $TEST_FILE_NAME && gosh compiler.scm $TEST_FILE_NAME 2>& /dev/null
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
run if-false 3 '(if #f 2 3)'
run no-else 2 '(if 1 2)'
run no-else2 nil '(if #f 2)'
run lambda-invoke 123 '((lambda (x) x) 123)'
run set-local 111 '((lambda (x)
                      (set! x 111)
                      x)
                    123)'
run closure 1 '(((lambda (x)
                   (lambda (y)
                     x))
                 1)
                2)'
run closure-set 23 '((lambda (x)
                       ((lambda ()
                          (set! x 23)))
                       x)
                     1)'
run call/cc 123 '(call/cc
                   (lambda (cc)
                     (cc 123)))'
run multiple-exp 3 '((lambda ()
                         1
                         2
                         3)
                       )'
run global-var 111 '((lambda ()
                       ((lambda ()
                          (set! global 111)))
                       global))'

# Test native functions
run cons '(1 . 2)' '(cons 1 2)'
run car '1' "(car '(1 2 3))"
run cdr '(2 3)' "(cdr '(1 2 3))"
run + '15' '(+ 1 2 3 4 5)'
run - '7' '(- 10 3)'
run negate '-10' '(- 10)'
run '*' '120' '(* 1 2 3 4 5)'
run / '3' '(/ 10 3)'

# Fail cases
fail unbound 'abc'
fail no-global '((lambda (x) y) 123)'


################################################################
# All tests succeeded.

rm $TEST_FILE_NAME
echo -n -e "\e[1;32mALL SUCCESS!\e[0m\n"

#
