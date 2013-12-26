#!/bin/sh

../yalp -L ../boot.bin code-walker.arc stat.arc -- ../boot.bin \
    | sort | uniq -c | sort -nr
