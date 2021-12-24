#include "../library/combinator.hpp"
#include <iostream>

int main()
{
    auto fact = [](auto f, auto n) -> int {
        return n == 0 ? 1 : n * f(f, n - 1);
    };
    auto gcd = [](auto f, auto a, auto b) -> int {
        return b == 0 ? a : f(f, b, a % b);
    };
    std::cout << fixed_point_combinator_multiple_arguments::Y(fact)(10) << std::endl;
    std::cout << fixed_point_combinator_multiple_arguments::Z(fact)(10) << std::endl;
    std::cout << fixed_point_combinator_multiple_arguments::Y(gcd)(1920, 1080) << std::endl;
    std::cout << fixed_point_combinator_multiple_arguments::Z(gcd)(1920, 1080) << std::endl;
}
