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

/* HACK: clang++ cannot compile these lambdas unless T is specified explicitly (although g++ can) */
template <class T>
class fixed_point_combinator_one_argument {
public:
    fixed_point_combinator_one_argument() = delete;
    static inline const auto Z = [](auto f) {
        auto foo = [f](auto x) -> T {
            auto bar = [x](auto &&y) {
                return x(x)(std::forward<decltype(y)>(y));
            };
            return f(bar);
        };
        return foo(foo);
    };
};

/* HACK: same as above */
template <class T>
class fixed_point_combinator_multiple_arguments {
public:
    fixed_point_combinator_multiple_arguments() = delete;
    static inline const auto Z = [](auto f) {
        auto foo = [f](auto x) -> T {
            auto bar = [x](auto &&...y) {
                return x(x)(std::forward<decltype(y)>(y)...);
            };
            return f(bar);
        };
        return foo(foo);
    };
};
