#include "combinator.hpp"
#include <cstdarg>

namespace {
    void looper_internal_one_argument(bool (*loopee)(va_list), va_list vl)
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
        fixed_point_combinator_one_argument<F>::Z(internal)(loopee)(vl);
    }
    void looper_internal_multiple_arguments(bool (*loopee)(va_list), va_list vl)
    {
        using F = std::function<void(decltype(loopee), decltype(vl))>;
        auto internal = [](auto f) {
            return [f](bool (*loopee)(va_list), va_list vl) {
                va_list stored;
                va_copy(stored, vl);
                if (loopee(vl)) {
                    f(loopee, stored);
                }
                va_end(stored);
            };
        };
        fixed_point_combinator_multiple_arguments<F>::Z(internal)(loopee, vl);
    }
}

extern "C" void looper(bool (*loopee)(va_list), ...)
{
    va_list vl;
    va_start(vl, loopee);
    //looper_internal_one_argument(loopee, vl);
    looper_internal_multiple_arguments(loopee, vl);
    va_end(vl);
    return;
}
