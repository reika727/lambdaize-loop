#include <stdio.h>

int main(void)
{
    int i;
    int x = 0;
    for (i = 0; i < 4096; ++i) {
        ++x;
    }
    //printf("%d\n", x);
    return 0;
}
