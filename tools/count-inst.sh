#!/bin/sh

../yalp -L ../boot.bin stat.arc -- ../boot.bin \
    | sort | uniq -c | sort -nr
