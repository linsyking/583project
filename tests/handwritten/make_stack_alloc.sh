#!/bin/bash

PATH2LIB="../build/VECPass/VECPass.so"

PASS=paddingpass

clang -emit-llvm -c stack_alloc.c -mavx2 -S -O3 
opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" stack_alloc.ll -S -o stack_alloc.paddingpass.ll