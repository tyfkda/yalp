#!/bin/sh

./yalp -L ./boot.bin tools/stat.yl ./boot.bin \
    | sort | uniq -c | sort -nr
