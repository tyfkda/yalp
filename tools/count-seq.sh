#!/bin/sh

../yalp -L ../boot.bin code-walker.yl seq.yl -- ../boot.bin \
    | sort | uniq -c | sort -nr
