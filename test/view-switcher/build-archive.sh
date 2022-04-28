#!/bin/bash

clang++ -ggdb view_switch.cpp -c -o view_switch.o
rm libkali.a
ar -crs libkali.a view_switch.o docfi.o
sudo cp libkali.a /usr/lib/


