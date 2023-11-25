#!/bin/bash -e
SCRIPTDIR=$(dirname "$(realpath "$0")")
BASENAME=$(basename "$1")
ORIGINAL_EXE=${BASENAME%.*}.out
OBFUSCATED_EXE=${BASENAME%.*}.obfuscated.out
set -x
make --directory="$SCRIPTDIR" "$ORIGINAL_EXE"
make --directory="$SCRIPTDIR" "$OBFUSCATED_EXE"
diff <("$SCRIPTDIR/$ORIGINAL_EXE" "${@:2}") <("$SCRIPTDIR/$OBFUSCATED_EXE" "${@:2}")
{ set +x; } 2>/dev/null
echo -e '\e[32mTEST SUCCEEDED\e[m'
