#include "combinator.hpp"
#include <cstdarg>

namespace {
    void looper_internal_multiple_arguments(bool (*loopee)(va_list), va_list vl)
    {
        auto internal = [](auto f, bool (*loopee)(va_list), va_list vl) -> void {
            va_list stored;
            va_copy(stored, vl);
            if (loopee(vl)) {
                f(f, loopee, stored);
            }
            va_end(stored);
        };
        fixed_point_combinator_multiple_arguments::Z(internal)(loopee, vl);
    }
    void looper_internal_one_argument(bool (*loopee)(va_list), va_list vl)
    {
        using F = higher_order_function<void, decltype(vl), decltype(loopee)>;
        auto internal = [](auto f) -> F {
            return [f](bool (*loopee)(va_list)) {
                return [f, loopee](va_list vl) {
                    va_list stored;
                    va_copy(stored, vl);
                    if (loopee(vl)) {
                        f(f)(loopee)(stored);
                    }
                    va_end(stored);
                };
            };
        };
        fixed_point_combinator_one_argument::Z(internal)(loopee)(vl);
    }
    void looper_internal_experimental(bool (*loopee)(va_list), va_list vl)
    {
        using F = higher_order_function<void, decltype(vl), decltype(loopee)>;
        auto internal = [](auto f) {
            return [f](bool (*loopee)(va_list)) {
                return [f, loopee](va_list vl) {
                    va_list stored;
                    va_copy(stored, vl);
                    if (loopee(vl)) {
                        f(loopee)(stored);
                    }
                    va_end(stored);
                };
            };
        };
        fixed_point_combinator_experimental<F>::Z(internal)(loopee)(vl);
    }
}

extern "C" void looper(bool (*loopee)(va_list), ...)
{
    va_list vl;
    va_start(vl, loopee);
    looper_internal_multiple_arguments(loopee, vl);
    //looper_internal_one_argument(loopee, vl);
    //looper_internal_experimental(loopee, vl);
    va_end(vl);
    return;
}
