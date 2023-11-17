/**
 * @file combinator.hpp
 * @brief 高階関数型と各種コンビネータの実装
 */

#include <functional>
#include <utility>

/**
 * @brief 高階関数型
 * @details 型 T1, ..., Tn から Tn -> (Tn-1 -> ... ->  (T2 -> T1)) を作成する
 */
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

/**
 * @see higher_order_function_t
 */
template<class... Types>
using higher_order_function = typename higher_order_function_t<Types...>::type;

/**
 * @brief 全ての引数を個別に扱う不動点コンビネータ
 */
template <class T>
class fixed_point_combinator_one_argument {
public:
    fixed_point_combinator_one_argument() = delete;

    /**
     * @brief Z コンビネータの実装
     */
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

/**
 * @brief 全ての引数をひとまとまりとして不動点コンビネータ
 */
template <class T>
class fixed_point_combinator_multiple_arguments {
public:
    fixed_point_combinator_multiple_arguments() = delete;

    /**
     * @brief Z コンビネータの実装
     */
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
