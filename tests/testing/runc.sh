#!/bin/bash

PATH2LIB="../../build/src/VECPass.so"

PASS=vecpass

clang -emit-llvm -c ${1}.c -mavx2 -S -O3

opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.ll -S -o ${1}.vecpass.ll

opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.ll -o ${1}.vecpass.bc

clang ${1}.vecpass.bc -o ${1}_vec

./${1}_vec

rm ${1}_vec ${1}.vecpass.bc
