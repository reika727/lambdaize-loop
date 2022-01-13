#include <stdio.h>

int main(void)
{
    int i, j, k;
    int x = 0;
    for (i = 0; i < 16; ++i) {
        for (j = 0; j < 16; ++j) {
            for (k = 0; k < 16; ++k) {
                ++x;
            }
        }
    }
    //printf("%d\n", x);
    return 0;
}
