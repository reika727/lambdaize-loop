#include <stdio.h>

int main(void)
{
    int i, j, k, l;
    int x = 0;
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            for (k = 0; k < 8; ++k) {
                for (l = 0; l < 8; ++l) {
                    ++x;
                }
            }
        }
    }
    //printf("%d\n", x);
    return 0;
}
