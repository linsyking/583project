#!/bin/bash

PATH2LIB="../../build/src/VECPass.so"

PASS=paddingpass

clang -emit-llvm -c stack_alloc3.c -mavx2 -S -O3 
opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" stack_alloc3.ll -S -o stack_alloc3.paddingpass.ll
