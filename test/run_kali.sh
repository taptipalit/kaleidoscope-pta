#!/bin/bash

set -x 

file="$1"
file_c="$1".c
file_bc="$file".bc
file_inst="$file"_inst.bc
file_inst_ll="$file"_inst.ll
file_discard="$file"_discard.bc
file_exe="$file".exe

clang -c -O0 $file_c -emit-llvm -o $file_bc

../Debug-build/bin/wpa -ander $file_bc

if [ $? -ne 0 ]; then
    echo "Failed to run partitioned pointer analysis"
    exit 1
fi

cp instrumented-module.bc $file_inst

llvm-dis $file_inst -o $file_inst_ll
opt -verify $file_inst -o $file_discard

if [ $? -ne 0 ]; then
    echo "Generated broken bitcode"
    exit 1
fi

clang++ -v $file_inst -lkali -o $file_exe # -L/data/tpalit/SVF/test/view_switcher 

if [ $? -ne 0 ]; then
    echo "Failed to link into binary"
    exit 1
fi
