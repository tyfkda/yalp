#!/bin/sh

./yalp tools/stat.yl boot.bin \
    | ./yalp tools/decode.yl \
    | sort | uniq -c | awk '{if ($1 > 1) { print }}' | sort -nr
