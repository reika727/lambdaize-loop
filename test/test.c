#include <stdio.h>

int multiple_loops(void)
{
    int a = 0;
    int i;
    for (i = 0; i < 10; ++i) {
        ++a;
    }
    for (i = 20; i > 0; --i) {
        ++a;
    }
    return a;
}

int return_in_loop(void)
{
    int i;
    for (i = 20; i < 100; ++i) {
        return 100;
    }
    return -1;
}

int switch_in_loop(void)
{
    int a = 0;
    int i;
    for (i = 0; i < 20; ++i) {
        switch (i) {
            case 0:
                a += 2;
                break;
            case 5:
                --a;
                break;
            case 10:
                a += a;
                break;
            case 15:
                a *= a;
                break;
            default:
                ++a;
                break;
        }
    }
    return a;
}

void double_nested_loop(void)
{
    int i, j;
    for (i = 1; i <= 9; ++i) {
        for (j = 1; j <= 9; ++j) {
            printf("%2d ", i * j);
        }
        puts("");
    }
    return;
}

int octuple_nested_loop(void)
{
    long long a = 0;
    int i, j, k, l, m, n, o, p;
    for (i = 0; i < 10; ++i) {
        for (j = 0; j < 10; ++j) {
            for (k = 0; k < 10; ++k) {
                for (l = 0; l < 10; ++l) {
                    for (m = 0; m < 10; ++m) {
                        for (n = 0; n < 10; ++n) {
                            for (o = 0; o < 10; ++o) {
                                for (p = 0; p < 10; ++p) {
                                    ++a;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return a;
}

int main(void)
{
    printf("%d\n", multiple_loops());
    printf("%d\n", return_in_loop());
    printf("%d\n", switch_in_loop());
    puts("QQ table");
    double_nested_loop();
    printf("%d\n", octuple_nested_loop());
    return 0;
}
