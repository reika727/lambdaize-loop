#!/bin/bash -e
SCRIPTDIR=$(dirname $(readlink -f $0))
BASENAME=$(basename -- $1)
EXTENSION=${BASENAME##*.}
ORIGINAL_LL=${1%.*}.ll
ORIGINAL_EXE=${1%.*}.out
OBFUSCATED_LL=${1%.*}.obfuscated.ll
OBFUSCATED_EXE=${1%.*}.obfuscated.out

if [ "$EXTENSION" = "c" ]; then
    set -x
    clang -S -emit-llvm -Xclang -disable-O0-optnone -o $ORIGINAL_LL $1
    clang -o $ORIGINAL_EXE $ORIGINAL_LL
elif [ "$EXTENSION" = "cpp" ]; then
    set -x
    clang++ -std=c++17 -S -emit-llvm -Xclang -disable-O0-optnone -o $ORIGINAL_LL $1
    clang++ -o $ORIGINAL_EXE $ORIGINAL_LL
else
    echo "invalid input"
    exit 1
fi
make -C $SCRIPTDIR/..
opt -S -load-pass-plugin $SCRIPTDIR/../lambdaize-loop.so -passes=lambdaize-loop -o $OBFUSCATED_LL $ORIGINAL_LL
clang++ -std=c++17 -o $OBFUSCATED_EXE $OBFUSCATED_LL $SCRIPTDIR/../library/looper.cpp
diff <($ORIGINAL_EXE "${@:2}") <($OBFUSCATED_EXE "${@:2}")
{ set +x; } 2>/dev/null
echo -e "\e[32mTEST SUCCEEDED\e[m"
