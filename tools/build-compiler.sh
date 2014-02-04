#!/bin/sh
# Build compiler.

./yalp -C compiler/boot.yl compiler/backquote.yl compiler/util.yl \
          compiler/setf.yl compiler/node.yl compiler/compiler.yl \
  | ./yalp tools/optimize.yl
