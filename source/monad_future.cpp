#include <functional>
#include <future>
#include <iostream>
#include <type_traits>
#include <optional>

template <typename F, typename A, typename B>
concept Func = std::is_invocable_r<B, F, A>::value;

template<template <typename>typename T>
struct is_monad {
    constexpr static bool value = false;
};

template<template <typename>typename TC>
constexpr bool is_monad_v = is_monad<TC>::value;

template<template <typename>typename TC>
concept Monad = is_monad_v<TC>;

template<template <typename>typename TC>
struct pure_t {
    template<typename A>
    constexpr TC<A> operator()(A a) const;
};

template<template <typename>typename TC >
constexpr auto pure = pure_t<TC>{};

template<>
template<typename A>
std::future<A> pure_t<std::future>::operator()(A a) const {
    return std::async(std::launch::deferred, [a](){ return a; });
}

template<typename A, typename B>
constexpr auto bind(std::future<A> fa) {
    return [fa = std::move(fa)]
        (Func<A, std::future<B>> auto f) mutable{
            return std::async(
                std::launch::deferred,
                [f, fa = std::move(fa)](std::future<A>) mutable{
                    return f(fa.get()).get();
                },
                std::move(fa)
            );
        };
}

template<>
struct is_monad<std::future> {
    constexpr static bool value = true;
};

template<
    template <typename>typename TC,
    typename A,
    typename B
>
    requires(Monad<TC>)
constexpr Func<TC<A>, TC<B>> auto _fmap(Func<A,B> auto f) {
    return [f](TC<A> fa) {
        return bind<A, B>(std::move(fa))
            ([f](A a){
                return pure<TC>(f(a));
            });
    };
}

int main() {
    constexpr auto f = [](int a) { return "hello world"; };
    constexpr auto listF = _fmap<std::future, int, const char *>(f);

    auto a = std::async(std::launch::deferred, [](){
        std::cout << "begin" << std::endl;
        return 1;
    });
    std::cout << "_1" << std::endl;
    auto b = listF(std::move(a));
    std::cout << "_2" << std::endl;

    std::cout << b.get() << std::endl;
}
