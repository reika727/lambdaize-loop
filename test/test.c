#include <stdio.h>

int foo(void)
{
    int b = 0;
    int i;
    for (i = 0; i < 20; ++i) {
        b += b + 1;
        printf("b is %d\n", b);
    }
    for (i = 20; i < 100; ++i) {
        return 100;
    }
    return -1;
}

void qq(void)
{
    int i, j;
    for (i = 1; i <= 9; ++i) {
        for (j = 1; j <= 9; ++j) {
            printf("%2d ", i * j);
        }
        puts("");
    }
}

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
    foo();
    qq();
    return 0;
}
