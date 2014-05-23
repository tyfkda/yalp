#!/bin/sh
# Build compiler.

set -e

./yalp -C compiler/boot.yl compiler/backquote.yl compiler/util.yl \
          compiler/setf.yl compiler/node.yl \
          compiler/match.yl compiler/code-walker.yl compiler/optimize.yl \
          compiler/compiler.yl
