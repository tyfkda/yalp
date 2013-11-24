#!/bin/sh

if [ $# == 0 ]; then
    echo "Argument required" >&2
    exit 1
fi

gosh -I compiler compiler/compiler.scm -c $@ | ./yalp
