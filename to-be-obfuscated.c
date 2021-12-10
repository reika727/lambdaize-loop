#include <stdio.h>

int main(void)
{
    int a = 0;
    int i;
    for (i = 0; i < 10; ++i) {
        ++a;
    }
    for (i = 0; i < 10; ++i) {
        ++a;
    }
    printf("%d\n", a);
    return 0;
}
