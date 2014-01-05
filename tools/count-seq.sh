#!/bin/sh

./yalp -L ./boot.bin tools/seq.yl ./boot.bin \
    | sort | uniq -c | sort -nr
