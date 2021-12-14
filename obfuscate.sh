#!/bin/sh
LL=${1%.*}.ll
OBFUSCATED=${1%.*}.obfuscated.ll
clang -S -emit-llvm -Xclang -disable-O0-optnone -o $LL $1
opt -S -load-pass-plugin ./lambdaize-loop.so -passes=lambdaize-loop -o $OBFUSCATED $LL
