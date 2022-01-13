#include <stdio.h>

int double_nested_loop(int x)
{
    int a = 0;
    int i, j;
    for (i = 0; i < x; ++i) {
        for (j = 0; j < x; ++j) {
            ++a;
        }
    }
    return a;
}

int main(void)
{
    printf("%d\n", double_nested_loop(100));
    return 0;
}
