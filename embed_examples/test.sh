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
  result=$(./$1)
  code=$?
  if [ $code -ne 0 ]; then
    error_exit "exit status is not 0 [$code]"
  fi
  if [ "$result" != "$2" ]; then
    error_exit "$2 expected, but got '$result'"
  fi
  echo ok
}

################################################################
# Test cases.

run 001_run_string 'Hello, Yalp!'
run 002_register_c_func '1234321'
run 003_call_script_func '3'
run 004_use_binder "1234321
123
** Foo bar baz **"

################################################################
# All tests succeeded.

echo -n -e "\033[1;32mVM-TEST ALL SUCCEEDED!\033[0;39m\n"
