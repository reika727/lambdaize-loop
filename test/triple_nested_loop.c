#include <stdio.h>

int main(void)
{
    int i, j, k;
    int x = 0;
    __attribute__((lambdaize_loop))
    for (i = 0; i < 16; ++i) {
        __attribute__((lambdaize_loop))
        for (j = 0; j < 16; ++j) {
            __attribute__((lambdaize_loop))
            for (k = 0; k < 16; ++k) {
                ++x;
            }
        }
    }
    //printf("%d\n", x);
    return 0;
}
