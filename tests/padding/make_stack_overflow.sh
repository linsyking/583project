#!/bin/bash

PATH2LIB="../../build/src/VECPass.so"

PASS=paddingpass

clang -emit-llvm -c stack_overflow.c -mavx2 -S -O3

case $1 in
    "padding")
        opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" stack_overflow.ll -S -o stack_overflow.paddingpass.ll
        llc stack_overflow.paddingpass.ll -o stack_overflow.paddingpass.s
        g++ -o stack_overflow stack_overflow.paddingpass.s -no-pie
        ;;

    "no-padding")
        llc stack_overflow.ll -o stack_overflow.s
        g++ -o stack_overflow stack_overflow.s -no-pie
        ;;
    *)
        exit 1
        ;;
esac
