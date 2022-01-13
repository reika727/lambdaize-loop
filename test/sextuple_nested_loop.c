#include <stdio.h>

int main(void)
{
    int i, j, k, l, m, n;
    int x = 0;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            for (k = 0; k < 4; ++k) {
                for (l = 0; l < 4; ++l) {
                    for (m = 0; m < 4; ++m) {
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
