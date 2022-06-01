#!/bin/bash

set -x 

file="$1"
file_c="$1".c
file_bc="$file".bc
file_linked="$file"_linked.bc
file_inst="$file"_inst.bc
file_inst_ll="$file"_inst.ll
file_discard="$file"_discard.bc
file_linked_inline="$file"_linked_inline.bc
file_linked_inline_ll="$file"_linked_inline.ll
file_exe="$file".exe

if [ -f $file_c ]
then
    clang -c -O0 -ggdb $file_c -emit-llvm -o $file_bc
fi


/home/tpalit/svf-kernel/Debug-build/bin/wpa -invariant-pwc=true \
-invariant-vgep=true -ander $file_bc

if [ $? -ne 0 ]; then
    echo "Failed to run invariant-based pointer analysis"
    exit 1
fi

cp instrumented-module.bc $file_inst

llvm-dis $file_inst -o $file_inst_ll
opt -verify $file_inst -o $file_discard


llvm-link $file_inst /home/tpalit/svf-kernel/test/view-switcher/docfi.bc /home/tpalit/svf-kernel/test/view-switcher/view_switch.bc -o $file_linked

if [ $? -ne 0 ]; then
    echo "Generated broken bitcode"
    exit 1
fi

opt -always-inline $file_linked -o $file_linked_inline

llvm-dis $file_linked_inline -o $file_linked_inline_ll

clang++ -v -ggdb $file_linked_inline -o $file_exe # -L/data/tpalit/SVF/test/view_switcher  # -lkali 

if [ $? -ne 0 ]; then
    echo "Failed to link into binary"
    exit 1
fi
