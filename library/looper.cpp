#include "combinator.hpp"
#include <cstdarg>

#ifndef MAX_RECURSION_COUNT
#define MAX_RECURSION_COUNT 512
#endif

namespace {
    void simple_while(bool (*loopee)(va_list), va_list vl)
    {
        while (true) {
            va_list stored;
            va_copy(stored, vl);
            bool result = loopee(stored);
            va_end(stored);
            if (!result) break;
        }
    }
    void z_combinator_one_argument(bool (*loopee)(va_list), va_list vl)
    {
        using F = higher_order_function<void, unsigned, decltype(vl), decltype(loopee)>;
        auto internal = [](auto f) {
            return [f](bool (*loopee)(va_list)) {
                return [f, loopee](va_list vl) {
                    return [f, loopee, vl](unsigned recursion_count) {
                        if (recursion_count < MAX_RECURSION_COUNT) {
                            va_list stored;
                            va_copy(stored, vl);
                            if (loopee(vl)) {
                                f(loopee)(stored)(recursion_count + 1);
                            }
                            va_end(stored);
                        } else {
                            simple_while(loopee, vl);
                        }
                    };
                };
            };
        };
        fixed_point_combinator_one_argument<F>::Z(internal)(loopee)(vl)(0);
    }
    void z_combinator_multiple_arguments(bool (*loopee)(va_list), va_list vl)
    {
        using F = std::function<void(decltype(loopee), decltype(vl), unsigned)>;
        auto internal = [](auto f) {
            return [f](bool (*loopee)(va_list), va_list vl, unsigned recursion_count) {
                if (recursion_count < MAX_RECURSION_COUNT) {
                    va_list stored;
                    va_copy(stored, vl);
                    if (loopee(vl)) {
                        f(loopee, stored, recursion_count + 1);
                    }
                    va_end(stored);
                } else {
                    simple_while(loopee, vl);
                }
            };
        };
        fixed_point_combinator_multiple_arguments<F>::Z(internal)(loopee, vl, 0);
    }
}

extern "C" void looper(bool (*loopee)(va_list), ...)
{
    va_list vl;
    va_start(vl, loopee);
    //simple_while(loopee, vl);
    //z_combinator_one_argument(loopee, vl);
    z_combinator_multiple_arguments(loopee, vl);
    va_end(vl);
    return;
}
