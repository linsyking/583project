#!/bin/bash

PATH2LIB="../build/src/VECPass.so"

PASS=vecpass

# Delete outputs from previous runs. Update this when you want to retain some files.
rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output

# Convert source code to bitcode (IR).
clang -emit-llvm -c ${1}.c -Xclang -disable-O0-optnone -o ${1}.bc
# llvm-as ${1}.ll

# Canonicalize natural loops (Ref: llvm.org/doxygen/LoopSimplify_8h_source.html)
opt -passes='loop-simplify' ${1}.bc -o ${1}.ls.bc

# Instrument profiler passes.
opt -passes='pgo-instr-gen,instrprof' ${1}.ls.bc -o ${1}.ls.prof.bc

# Note: We are using the New Pass Manager for these passes! 

# Generate binary executable with profiler embedded
clang -fprofile-instr-generate ${1}.ls.prof.bc -o ${1}_prof


echo "----Origin Output-----"
# ./${1}_prof> /dev/null
./${1}_prof>correct
echo "----LLVM Pass Output-----"

# Converting it to LLVM form. This step can also be used to combine multiple profraw files,
# in case you want to include different profile runs together.
llvm-profdata merge -o ${1}.profdata default.profraw

# The "Profile Guided Optimization Use" pass attaches the profile data to the bc file.
opt -passes="pgo-instr-use" -o ${1}.profdata.bc -pgo-test-profile-file=${1}.profdata < ${1}.ls.bc > /dev/null

# We now use the profile augmented bc file as input to your pass.
opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.profdata.bc -o ${1}.vecpass.bc

clang ${1}.vecpass.bc -O2 -o ${1}_vecpass

echo "----Optimized Output-----"
# Produce output from binary to check correctness
./${1}_vecpass>output

diff correct output

llvm-dis ${1}.bc
llvm-dis ${1}.vecpass.bc

# Cleanup: Remove this if you want to retain the created files. And you do need to.
rm -f default.profraw *_prof *.bc *.profdata *_output correct output
