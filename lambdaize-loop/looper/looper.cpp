/**
 * @file looper.cpp
 * @brief looper 関数の実装
 */

#include "combinator.hpp"
#include <cstdarg>

#ifndef MAX_RECURSION_COUNT
#define MAX_RECURSION_COUNT 8192
#endif

namespace {
    /**
     * @brief 再帰せずに繰り返し処理を行う
     * @param loopee 繰り返し対象の関数へのポインタ
     * @param vl loopee への引数
     */
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

    /**
     * @brief 全ての変数を個別に扱う Z コンビネータで再帰を行う
     * @param loopee 繰り返し対象の関数へのポインタ
     * @param vl loopee への引数
     * @note 再帰回数が MAX_RECURSION_COUNT に達した場合は simple_while に移行する
     */
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

    /**
     * @brief 全ての変数をひとまとまりで扱う Z コンビネータで再帰を行う
     * @param loopee 繰り返し対象の関数へのポインタ
     * @param vl loopee への引数
     * @note 再帰回数が MAX_RECURSION_COUNT に達した場合は simple_while に移行する
     */
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

/**
 * @brief looper 関数
 * @param loopee 繰り返し対象の関数へのポインタ
 * @param ... loopee への引数
 */
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
