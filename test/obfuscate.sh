#!/bin/sh -e
SCRIPTDIR=$(dirname $(readlink -f $0))
LL=${1%.*}.ll
OBFUSCATED=${1%.*}.obfuscated.ll
make -C $SCRIPTDIR/..
clang -S -emit-llvm -Xclang -disable-O0-optnone -o $LL $1
opt -S -load-pass-plugin $SCRIPTDIR/../lambdaize-loop.so -passes=lambdaize-loop -o $OBFUSCATED $LL
