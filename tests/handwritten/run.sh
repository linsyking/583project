#!/bin/bash

PATH2LIB="../../build/src/VECPass.so"

PASS=vecpass

clang ${1}.c -O3 -o ${1}

# ./${1}

clang -emit-llvm -c ${1}.c -mavx2 -S -O3 

# llvm-as ${1}.ll

opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.ll -S -o ${1}.vecpass.ll

opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.ll -o ${1}.vecpass.bc

clang ${1}.vecpass.bc -fstack-protector-strong -o ${1}_vec

./${1}_vec

rm ${1}_vec ${1}.vecpass.bc ${1}
