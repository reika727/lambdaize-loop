#include <cstdarg>
#include <functional>
#include <utility>

namespace {
    template<class T, class U, class... Types>
    struct higher_order_function_t {
        using type = typename higher_order_function_t<typename higher_order_function_t<T, U>::type, Types...>::type;
        higher_order_function_t() = delete;
    };
    template<class T, class U>
    struct higher_order_function_t<T, U> {
        using type = std::function<T(U)>;
        higher_order_function_t() = delete;
    };
    template<class... Types>
    using higher_order_function = typename higher_order_function_t<Types...>::type;
    class fixed_point_combinator_multiple_arguments {
    public:
        static inline const auto Y = [](auto f) {
            return [=](auto &&...x) {
                return f(f, std::forward<decltype(x)>(x)...);
            };
        };
        static inline const auto Z = [](auto f) {
            auto curry = [](auto f) {
                return [=](auto &&...x) {
                    return [&](auto &&...y) {
                        return f(std::forward<decltype(x)>(x)..., std::forward<decltype(y)>(y)...);
                    };
                };
            };
            return curry(f)(f);
        };
    };
    class fixed_point_combinator_one_argument {
    public:
        static inline const auto Y = [](auto f) {
            return [=](auto &&x) {
                return f(f)(x);
            };
        };
        static inline const auto Z = [](auto f) {
            auto curry = [](auto f) {
                return [=](auto &&x) {
                    return [&](auto &&y) {
                        return f(std::forward<decltype(x)>(x))(std::forward<decltype(y)>(y));
                    };
                };
            };
            return curry(f)(f);
        };
    };
    template <class T>
    class fixed_point_combinator_experimental {
    public:
        /* Y = (lf.(lx.f(xx))(lx.f(xx))) */
        [[deprecated("Y combinator causes infinite recursion. Use Z combinator!")]]
        static inline const auto Y = [](auto f) {
            auto foo = [f](auto x) -> T {
                return f(x(x));
            };
            return foo(foo);
        };
        /* Z = (lf.(lx.f(ly.xxy))(lx.f(ly.xxy))) */
        static inline const auto Z = [](auto f) {
            auto foo = [f](auto x) -> T {
                auto bar = [x](auto y) {
                    return x(x)(y);
                };
                return f(bar);
            };
            return foo(foo);
        };
    };
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
