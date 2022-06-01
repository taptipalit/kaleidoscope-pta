#!/bin/bash

set -x 

file="$1"
file_c="$1".c
file_bc="$file".bc
file_inst="$file"_noinv_inst.bc
file_inst_ll="$file"_no_inv_inst.ll
file_discard="$file"_discard.bc
file_exe="$file"_unmod_.exe

if [ -f $file_c ]
then
    clang -c -O0 -ggdb $file_c -emit-llvm -o $file_bc
fi


clang++ -v -ggdb $file_bc -o $file_exe # -L/data/tpalit/SVF/test/view_switcher 

if [ $? -ne 0 ]; then
    echo "Failed to link into binary"
    exit 1
fi

