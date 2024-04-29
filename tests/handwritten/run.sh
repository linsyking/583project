#!/bin/bash

PATH2LIB="../../build/src/VECPass.so"

PASS=vecpass

clang -emit-llvm -c ${1}.c -mavx2 -S -O3 

opt -load-pass-plugin=../../build/src/VECPass.so -passes=scalarize ${1}.ll -o ${1}.scalarized.ll

clang ${1}.scalarized.ll -o ${1}_scalarized

echo "Running scalarized version"

time ./${1}_scalarized > ${1}_scalarized_output.txt

opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.ll -S -o ${1}.vecpass.ll

clang ${1}.vecpass.ll -o ${1}_vec

echo "Running vectorized version"

time ./${1}_vec > ${1}_vec_output.txt

diff ${1}_vec_output.txt ${1}_scalarized_output.txt

rm ${1}_vec ${1}.vecpass.ll ${1}.ll ${1}.scalarized.ll ${1}_scalarized ${1}_vec_output.txt ${1}_scalarized_output.txt
