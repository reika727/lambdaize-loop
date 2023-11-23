#include <stdio.h>

int main(void)
{
    int i, j, k, l, m, n;
    int x = 0;
    __attribute__((lambdaize_loop))
    for (i = 0; i < 4; ++i) {
        __attribute__((lambdaize_loop))
        for (j = 0; j < 4; ++j) {
            __attribute__((lambdaize_loop))
            for (k = 0; k < 4; ++k) {
                __attribute__((lambdaize_loop))
                for (l = 0; l < 4; ++l) {
                    __attribute__((lambdaize_loop))
                    for (m = 0; m < 4; ++m) {
                        __attribute__((lambdaize_loop))
                        for (n = 0; n < 4; ++n) {
                            ++x;
                        }
                    }
                }
            }
        }
    }
    //printf("%d\n", x);
    return 0;
}
