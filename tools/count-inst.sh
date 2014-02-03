#!/bin/sh

./yalp -L boot.bin tools/stat.yl boot.bin \
    | ./yalp -L boot.bin tools/decode.yl \
    | sort | uniq -c | awk '{if ($1 > 1) { print }}' | sort -nr
