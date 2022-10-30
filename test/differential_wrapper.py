#!/bin/bash

BC="$1"
dt=`date '+%d_%m_%Y_%H_%M_%S'`
/home/tpalit/svf-latest/SVF/Debug-build/bin/dvf -cxt -query=funptr -max-cxt=30 -flow-bg=1000 -cxt-bg=1000 -print-query-pts=true -dump-callgraph -print-fp -ander $BC 2>&1 | tee $dt.precise.$BC.out 
/home/tpalit/svf-kernel/Debug-build/bin/wpa -invariant-pwc=true -log-all=false -invariant-vgep=true  -short-circuit=true -ptd=persistent -ander $BC 2>&1 | tee $dt.kaleidoscope.$BC.out


../differential.py $dt.precise.$BC.out $dt.kaleidoscope.$BC.out
