#!/bin/bash

PATH2LIB="../../build/src/VECPass.so"

PASS=vecpass

clang -emit-llvm -c ${1}.c -mavx2 -g -S -O3

# llvm-as ${1}.ll

opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.ll -S -o ${1}.vecpass.ll
