#!/bin/bash

SCRIPTDIR=$(dirname "$(realpath "$0")")

function dump_instruction_counts() {
    objdump --disassemble --no-show-raw-insn "$1" |
    grep '^ '                                     |
    cut --fields=2                                |
    cut --delimiter=' ' --fields=1                |
    sort                                          |
    uniq --count
}

awk --assign filename1="$(basename "$1")"   \
    --assign filename2="$(basename "$2")"   \
    --file "$SCRIPTDIR"/dump-instdist.awk \
    <(dump_instruction_counts "$1")       \
    <(dump_instruction_counts "$2")       |
gnuplot --persist -e "$(tr '\n' ';' < "$SCRIPTDIR"/plot.gpi)"
