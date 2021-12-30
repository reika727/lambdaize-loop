#include <stdio.h>

int hundred_million_times_loop(void)
{
    long long a = 0;
    int i;
    for (i = 0; i < 100000000; ++i) {
        ++a;
    }
    return a;
}

int main(void)
{
    printf("%d\n", hundred_million_times_loop());
    return 0;
}
