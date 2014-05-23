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
  result=$(../yalp $1.yl)
  if [ "$result" != "$2" ]; then
    error_exit "$2 expected, but got '$result'"
  fi
  echo ok
}

################################################################
# Test cases.

run hello 'Hello, world!'
run fib '55'
run amb '(4 3 5)'

################################################################
# All tests succeeded.

echo -n -e "\033[1;32mEXAMPLE-TEST ALL SUCCEEDED!\033[0;39m\n"
