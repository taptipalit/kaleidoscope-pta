#!/bin/bash

clang++ -emit-llvm view_switch.cpp -c -o view_switch.bc
clang++ -emit-llvm -ggdb docfi.cpp -c -o docfi.bc


