#!/bin/bash

set -x 

file="$1"
file_c="$1".c
file_bc="$file".bc
file_linked="$file"_linked.bc
file_linked_inline="$file"_linked_inline.bc
file_inst="$file"_noinv_inst.bc
file_inst_ll="$file"_no_inv_inst.ll
file_discard="$file"_discard.bc
file_exe="$file"_noinv_.exe

if [ -f $file_c ]
then
    clang -c -O0 -ggdb $file_c -emit-llvm -o $file_bc
fi

/home/tpalit/svf-kernel/Debug-build/bin/wpa -invariant-pwc=true \
    -invariant-vgep=true -no-invariants=true -ander $file_bc

if [ $? -ne 0 ]; then
    echo "Failed to run invariant-based pointer analysis"
    exit 1
fi

cp instrumented-module.bc $file_inst

llvm-dis $file_inst -o $file_inst_ll
opt -verify $file_inst -o $file_discard



if [ $? -ne 0 ]; then
    echo "Generated broken bitcode"
    exit 1
fi


llvm-link $file_inst /home/tpalit/svf-kernel/test/view-switcher/docfi.bc -o $file_linked

opt -always-inline $file_linked -o $file_linked_inline

clang++ -v -ggdb $file_linked_inline -o $file_exe # -L/data/tpalit/SVF/test/view_switcher 

if [ $? -ne 0 ]; then
    echo "Failed to link into binary"
    exit 1
fi

