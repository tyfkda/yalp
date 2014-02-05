#!/bin/sh
# Build compiler and convert its data into c++ source.

DST=src/boot.cc
TMP=boot.cc
BOOT_BIN=obj/boot.bin

./tools/build-compiler.sh > ${BOOT_BIN}

echo "// This file is generated from ${BOOT_BIN}
namespace yalp {
extern const char bootBinaryData[];
const char bootBinaryData[] = " > ${TMP}

ruby -ne 'puts %!"#{$_.chomp.gsub(/[\\"]/, {"\\"=>"\\\\", "\""=>"\\\""})}\\n"!' < ${BOOT_BIN} >> ${TMP}

echo ";
}  // namespace yalp" >> ${TMP}

diff=$(diff ${TMP} ${DST})
if [ $? != 0 ]; then
    cp ${TMP} ${DST}
    echo "Updated"
else
    rm -rf ${TMP}
    echo "Unchanged"
fi
