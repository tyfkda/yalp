#!/bin/sh
# Build compiler and convert its data into c++ source.

DST=src/boot.cc
BOOT_BIN=obj/boot.bin

./tools/build-compiler.sh > ${BOOT_BIN}

echo "// This file is generated from ${BOOT_BIN}
namespace yalp {
extern const char bootBinaryData[];
const char bootBinaryData[] = " > ${DST}

ruby -ne 'puts %!"#{$_.chomp.gsub(/[\\"]/, {"\\"=>"\\\\", "\""=>"\\\""})}\\n"!' < ${BOOT_BIN} >> ${DST}

echo ";
}  // namespace yalp" >> ${DST}
