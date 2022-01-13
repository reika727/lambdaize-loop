#include <stdio.h>

int octuple_nested_loop(int x)
{
    long long a = 0;
    int i, j, k, l, m, n, o, p;
    for (i = 0; i < x; ++i) {
        for (j = 0; j < x; ++j) {
            for (k = 0; k < x; ++k) {
                for (l = 0; l < x; ++l) {
                    for (m = 0; m < x; ++m) {
                        for (n = 0; n < x; ++n) {
                            for (o = 0; o < x; ++o) {
                                for (p = 0; p < x; ++p) {
                                    ++a;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return a;
}

int main(void)
{
    printf("%d\n", octuple_nested_loop(10));
    return 0;
}
