#!/bin/bash

set -x 

file="$1"
options="$2"

file_c="$1".c
file_bc="$file".bc
file_linked="$file"_linked.bc
file_inst="$file"_inst.bc
file_inst_ll="$file"_inst.ll
file_discard="$file"_discard.bc
file_linked_inline="$file"_linked_inline.bc
file_linked_inline_ll="$file"_linked_inline.ll
file_exe="$file".exe

#$SVF_HOME=/home/tpalit/svf-kernel/
SVF_HOME=/home/tpalit/svf-kernel/
#/morespace/introspection/svf-kernel

if [ -f $file_c ]
then
    clang -c -O0 -ggdb $file_c -emit-llvm -o $file_bc
    llvm-dis $file_bc
fi


/home/tpalit/svf-kernel/Debug-build/bin/wpa -invariant-pwc=true \
-invariant-vgep=true -log-all=false -dump-constraint-graph=false -prevent-collapse-explosion=false -short-circuit=true \
-ptd=persistent \
-lander $file_bc #aeSearchNearestTimer,

#/home/tpalit/svf-latest/SVF/Debug-build/bin/wpa -dump-constraint-graph=true -stat-limit=1 \
#-ptd=persistent -print-all-pts=false \
#-lander $file_bc #aeSearchNearestTimer,

#-print-all-pts -debug-funcs=initServer #-field-limit=30

if [ $? -ne 0 ]; then
    echo "Failed to run invariant-based pointer analysis"
    exit 1
fi

cp instrumented-module.bc $file_inst

llvm-dis $file_inst -o $file_inst_ll
opt -verify $file_inst -o $file_discard

clang++ -c -O0 -ggdb -emit-llvm $SVF_HOME/test/view-switcher/view_switch.cpp -o $SVF_HOME/test/view-switcher/view_switch.bc
clang++ -c -O0 -ggdb -emit-llvm $SVF_HOME/test/view-switcher/debugger.cpp -o $SVF_HOME/test/view-switcher/debugger.bc
clang++ -c -O0 -ggdb -emit-llvm $SVF_HOME/test/view-switcher/docfi.cpp -o $SVF_HOME/test/view-switcher/docfi.bc

llvm-link $file_inst $SVF_HOME/test/view-switcher/docfi.bc $SVF_HOME/test/view-switcher/view_switch.bc $SVF_HOME/test/view-switcher/debugger.bc -o $file_linked

if [ $? -ne 0 ]; then
    echo "Generated broken bitcode"
    exit 1
fi

#opt -always-inline $file_linked -o $file_linked_inline
cp $file_linked $file_linked_inline

llvm-dis $file_linked_inline -o $file_linked_inline_ll

clang++ -v -ggdb $file_linked_inline -o $file_exe $options # -L/data/tpalit/SVF/test/view_switcher  # -lkali 

if [ $? -ne 0 ]; then
    echo "Failed to link into binary"
    exit 1
fi
