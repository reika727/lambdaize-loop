#!/bin/bash
while getopts n: OPT
do
    case $OPT in
        "n" ) TIMES="$OPTARG" ;;
    esac
done
shift $(($OPTIND - 1))
awk -v TIMES="$TIMES" '{printf "%s %.6f\n", $1, $2 / TIMES}' <((time -p for ((i=0; i<"$TIMES"; ++i)); do "$@"; done) 2>&1)
