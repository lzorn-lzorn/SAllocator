#pragma once
#include <type_traits>
#include <utility>
/*
    * 编译期for循环
    * Usage: static_for<Beg, End>([&](size_t i){
    *    i 是 循环变量
    * });
    * 其会在编译期将该 static_for 展开, 形成 End - Beg 行 [](){}
    */
template <size_t Beg, size_t End, class Lambda>
void static_for(Lambda lambda) {
    if constexpr (Beg < End) {
        std::integral_constant<size_t, Beg> i;
        struct Breaker {
            bool *m_broken;
            constexpr void static_break() const {
                *m_broken = true;
            }
        };
        if constexpr (std::is_invocable_v<Lambda, decltype(i), Breaker>) {
            bool broken = false;
            lambda(i, Breaker{&broken});
            if (broken) {
                return;
            }
        } else {
            lambda(i);
        }
        static_for<Beg + 1, End>(lambda);
    }
}

template <size_t ...Is, class Lambda>
void _static_for_impl(Lambda lambda, std::index_sequence<Is...>) {
    (lambda(std::integral_constant<size_t, Is>{}), ...);
    /* std::make_index_sequence<4>; */
    /* std::index_sequence<0, 1, 2, 3>; */
}

template <size_t N, class Lambda>
void static_for(Lambda lambda) {
    _static_for_impl(lambda, std::make_index_sequence<N>{});
}
