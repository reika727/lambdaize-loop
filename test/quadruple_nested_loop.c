#include <stdio.h>

int main(void)
{
    int i, j, k, l;
    int x = 0;
    __attribute__((lambdaize_loop))
    for (i = 0; i < 8; ++i) {
        __attribute__((lambdaize_loop))
        for (j = 0; j < 8; ++j) {
            __attribute__((lambdaize_loop))
            for (k = 0; k < 8; ++k) {
                __attribute__((lambdaize_loop))
                for (l = 0; l < 8; ++l) {
                    ++x;
                }
            }
        }
    }
    //printf("%d\n", x);
    return 0;
}
