#!/bin/bash -e
SCRIPTDIR=$(dirname "$(realpath "$0")")
set -x
make --directory="$SCRIPTDIR"
opt -load-pass-plugin "$SCRIPTDIR"/count-cyclomatic-complexity.so -passes=count-cyclomatic-complexity "$1"
