#!/bin/zsh

TEST_FILE_NAME='test.arc'
BIN_FILE_NAME='test.bin'

################################################################
# Test framework.

function fail() {
  echo -n -e '\e[1;31m[ERROR]\e[0m '
  echo "$1"
  exit 1
}

function run() {
    echo -n "Testing $1 ... "
    result=$(echo "$3" > $TEST_FILE_NAME && gosh -I ../compiler ../compiler/compiler.scm -c $TEST_FILE_NAME > $BIN_FILE_NAME && ../yalp $BIN_FILE_NAME)
    if [ "$result" != "$2" ]; then
        echo FAILED
        fail "$2 expected, but got '$result'"
    fi
    echo ok
}

################################################################
# Test cases.

run integer 123 '123'
run quote abc '(quote abc)'
run if-true 2 '(if 1 2 3)'
run if-false 3 '(if () 2 3)'
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


################################################################
# All tests succeeded.

#rm $TEST_FILE_NAME
echo -n -e "\e[1;32mALL SUCCESS!\e[0m\n"

#
