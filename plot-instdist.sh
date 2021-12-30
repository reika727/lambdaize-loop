#!/bin/bash

gawk '
function comp_by_ratio(i1, v1, i2, v2)
{
    c11 = inst_count1[i1] / sum1
    c12 = inst_count1[i2] / sum1
    c21 = inst_count2[i1] / sum2
    c22 = inst_count2[i2] / sum2
    if (c21 != 0 && c22 != 0) {
        ratio1 = c11 / c21
        ratio2 = c12 / c22
        return ratio1 < ratio2 ? -1 : (ratio1 > ratio2 ? 1 : 0)
    } else if (c21 != 0) {
        return -1
    } else if (c22 != 0) {
        return 1
    } else {
        return c11 < c12 ? -1 : (c11 > c12 ? 1 : 0)
    }
}
NR == FNR {
    inst_count1[$2] = $1
    sum1 += $1
    next
}
{
    inst_count2[$2] = $1
    sum2 += $1
}
END {
    for (inst in inst_count1) {
        inst_set[inst] = 0
        if (inst_count2[inst] == "") {
            inst_count2[inst] = 0
        }
    }
    for (inst in inst_count2) {
        inst_set[inst] = 0
        if (inst_count1[inst] == "") {
            inst_count1[inst] = 0
        }
    }
    n = asorti(inst_set, sorted, "comp_by_ratio");
    for (i = 1; i <= n; ++i) {
        inst = sorted[i]
        printf "%f \"%s\"\n", inst_count1[inst] * 100 / sum1, inst
    }
    print "e"
    for (i = 1; i <= n; ++i) {
        inst = sorted[i]
        printf "%f \"%s\"\n", inst_count2[inst] * 100 / sum2, inst
    }
    print "e"
}' \
    <(objdump -d --no-show-raw-insn $1 | grep '^ ' | cut -f 2 | cut -d ' ' -f 1 | sort | uniq -c)\
    <(objdump -d --no-show-raw-insn $2 | grep '^ ' | cut -f 2 | cut -d ' ' -f 1 | sort | uniq -c)\
| gnuplot -p -e "
set style data histogram;
set ylabel 'percentage';
set xtic rotate by 292;
plot '-' using 1:xtic(2) w lp pt 2 ps 2 title \"$1\", '' using 1:xtic(2) w lp pt 2 ps 2 title \"$2\""
