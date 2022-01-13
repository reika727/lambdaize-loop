#include <stdio.h>

int main(void)
{
    int i, j;
    int x = 0;
    for (i = 0; i < 64; ++i) {
        for (j = 0; j < 64; ++j) {
            ++x;
        }
    }
    //printf("%d\n", x);
    return 0;
}
