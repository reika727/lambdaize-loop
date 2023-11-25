#!/bin/bash
while getopts n: OPT
do
    case $OPT in
        "n" ) TIMES="$OPTARG" ;;
         *  ) echo "usage: $0 -n TIMES"; exit 1 ;;
    esac
done
shift $((OPTIND - 1))
awk --assign TIMES="$TIMES" '{printf "%s %.6f\n", $1, $2 / TIMES}' <((time -p for ((i=0; i<"$TIMES"; ++i)); do "$@"; done) 2>&1)
