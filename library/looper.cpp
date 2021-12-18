#include <cstdarg>
#include <utility>
namespace {
    void looper_internal_recursion(bool (*loopee)(va_list), va_list vl)
    {
        va_list stored;
        va_copy(stored, vl);
        if (loopee(vl)) {
            looper_internal_recursion(loopee, stored);
        }
        va_end(stored);
        return;
    }
    void looper_internal_lambda_recursion(bool (*loopee)(va_list), va_list vl)
    {
        auto Z = [](auto f) {
            auto curry = [](auto f) {
                return [=](auto &&...xs) {
                    return [&](auto &&...ys) {
                        return f(std::forward<decltype(xs)>(xs)..., std::forward<decltype(ys)>(ys)...);
                    };
                };
            };
            return curry(f)(f);
        };
        Z([](auto f, bool (*loopee)(va_list), va_list vl) -> void {
            va_list stored;
            va_copy(stored, vl);
            if (loopee(vl)) {
                f(f, loopee, stored);
            }
            va_end(stored);
            return;
        })
        (loopee, vl);
    }
}
extern "C" void looper(bool (*loopee)(va_list), ...)
{
    va_list vl;
    va_start(vl, loopee);
    looper_internal_lambda_recursion(loopee, vl);
    va_end(vl);
    return;
}
