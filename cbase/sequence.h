#pragma once

#include <tuple>

template <int... N>
struct sequence {};

template <int... N>
struct seq_generator;

template <int N, int... Seq>
struct seq_generator<N, Seq...> {
    using type = typename seq_generator<N - 1, N - 1, Seq...>::type;
};

template <int... Seq>
struct seq_generator<0, Seq...> {
    using type = sequence<Seq...>;
};

template <int N>
using sequence_t = typename seq_generator<N>::type;

template <typename F, typename... Args, int... N>
void func_call_helper(F&& f, std::tuple<Args...>&& args, sequence<N...>&&) {
    (std::forward<F>(f))(
        std::get<N>(std::forward<std::tuple<Args...>>(args))...);
}

template <typename F, typename... Args>
void func_call(F&& f, std::tuple<Args...>&& args) {
    func_call_helper(std::forward<F>(f),
                     std::forward<std::tuple<Args...>>(args),
                     sequence_t<sizeof...(Args)>{});
}

/*
template <typename F, typename... Args>
void func_call(F&& f, Args&&... args) {
    (std::forward<F>(f))(std::forward<Args>(args)...);
}
*/
