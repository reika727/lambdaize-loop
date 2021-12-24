#include <functional>
#include <utility>

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
    fixed_point_combinator_multiple_arguments() = delete;
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
    fixed_point_combinator_one_argument() = delete;
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
    fixed_point_combinator_experimental() = delete;
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
