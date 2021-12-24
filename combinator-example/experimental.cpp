#include "../library/combinator.hpp"
#include <iostream>

int main()
{
    using fact_type = higher_order_function<int, int>;
    auto fact = [](auto f) {
        return [f](auto n) {
            return n == 0 ? 1 : n * f(n - 1);
        };
    };
    using gcd_type = higher_order_function<int, int, int>;
    auto gcd = [](auto f) {
        return [f](int a) {
            return [f, a](int b) {
                return b == 0 ? a : f(b)(a % b);
            };
        };
    };
    //std::cout << fixed_point_combinator_experimental<fact_type>::Y(fact)(10) << std::endl;
    std::cout << fixed_point_combinator_experimental<fact_type>::Z(fact)(10) << std::endl;
    //std::cout << fixed_point_combinator_experimental<gcd_type>::Y(gcd)(1920)(1080) << std::endl;
    std::cout << fixed_point_combinator_experimental<gcd_type>::Z(gcd)(1920)(1080) << std::endl;
}
