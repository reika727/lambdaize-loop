#include <stdio.h>

int main(void)
{
    int i, j;
    int x = 0;
    __attribute__((lambdaize_loop))
    for (i = 0; i < 64; ++i) {
        __attribute__((lambdaize_loop))
        for (j = 0; j < 64; ++j) {
            ++x;
        }
    }
    //printf("%d\n", x);
    return 0;
}
