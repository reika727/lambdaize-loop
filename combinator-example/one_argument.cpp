#include "../library/combinator.hpp"
#include <iostream>

int main()
{
    auto fact = [](auto f) -> higher_order_function<int, int> {
        return [f](auto n) {
            return n == 0 ? 1 : n * f(f)(n - 1);
        };
    };
    auto gcd = [](auto f) -> higher_order_function<int, int, int> {
        return [f](int a) {
            return [f, a](int b) {
                return b == 0 ? a : f(f)(b)(a % b);
            };
        };
    };
    std::cout << fixed_point_combinator_one_argument::Y(fact)(10) << std::endl;
    std::cout << fixed_point_combinator_one_argument::Z(fact)(10) << std::endl;
    std::cout << fixed_point_combinator_one_argument::Y(gcd)(1920)(1080) << std::endl;
    std::cout << fixed_point_combinator_one_argument::Z(gcd)(1920)(1080) << std::endl;
}
