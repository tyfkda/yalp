#!/bin/sh

../yalp -L ../boot.bin code-walker.yl stat.yl -- ../boot.bin \
    | sort | uniq -c | sort -nr
