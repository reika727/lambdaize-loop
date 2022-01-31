#!/bin/bash -e
SCRIPTDIR=$(dirname $(readlink -f $0))
set -x
make -C $SCRIPTDIR
opt -load-pass-plugin $SCRIPTDIR/count-cyclomatic-complexity.so -passes=count-cyclomatic-complexity $1
