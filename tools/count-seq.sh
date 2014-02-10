#!/bin/sh

./yalp tools/seq.yl boot.bin \
    | sort | uniq -c | awk '{if ($1 > 1) { print }}' | sort -nr
