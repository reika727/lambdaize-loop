#!/bin/bash -e
SCRIPTDIR=$(dirname $(readlink -f $0))
ORIGINAL_LL=${1%.*}.ll
ORIGINAL_EXE=${1%.*}.out
OBFUSCATED_LL=${1%.*}.obfuscated.ll
OBFUSCATED_EXE=${1%.*}.obfuscated.out
set -x
make -C $SCRIPTDIR/..
clang -S -emit-llvm -Xclang -disable-O0-optnone -o $ORIGINAL_LL $1
clang -o $ORIGINAL_EXE $ORIGINAL_LL
opt -S -load-pass-plugin $SCRIPTDIR/../lambdaize-loop.so -passes=lambdaize-loop -o $OBFUSCATED_LL $ORIGINAL_LL
clang -o $OBFUSCATED_EXE $OBFUSCATED_LL $SCRIPTDIR/../library/*
diff <($ORIGINAL_EXE) <($OBFUSCATED_EXE)
{ set +x; } 2>/dev/null
echo -e "\e[32mTEST SUCCEEDED\e[m"
