#!/bin/bash

PATH2LIB="../../build/src/VECPass.so"

PASS=vecpass

opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.ll -S -o ${1}.vecpass.ll
