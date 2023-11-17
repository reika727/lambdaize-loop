int main()
{
    __attribute__((lambdaize_loop))
    for (int i = 0; i < 10; ++i) {
    }

    for (int i = 0; i < 20; ++i) {
    }

    __attribute__((lambdaize_loop))
    for (int i = 0; i < 10; ++i) {
    }

    for (int i = 0; i < 20; ++i) {
    }

    __attribute__((lambdaize_loop))
    for (int i = 0; i < 10; ++i) {
    }

    for (int i = 0; i < 20; ++i) {
    }

    return 0;
}
