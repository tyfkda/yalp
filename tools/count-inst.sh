#!/bin/sh

./yalp -L ./boot.bin tools/stat.yl ./boot.bin \
    | sort | uniq -c | awk '{if ($1 > 1) { print }}' | sort -nr
