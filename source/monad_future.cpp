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

template<template <typename>typename TC>
struct bind_t {
    template<typename A, typename B>
    constexpr auto operator()(TC<A> _a) const;
};

template<template <typename>typename TC >
constexpr auto bind = bind_t<TC>{};

template<
    template <typename>typename TC,
    typename A,
    typename B
>
    requires(Monad<TC>)
constexpr Func<TC<A>, TC<B>> auto _fmap(Func<A,B> auto f) {
    return [f](TC<A> fa) {
        return bind<TC>.template operator()<A, B>(std::move(fa))
            ([f](A a){
                return pure<TC>(f(a));
            });
    };
}

template<>
template<typename A>
std::future<A> pure_t<std::future>::operator()(A a) const {
    return std::async(std::launch::deferred, [a](){ return a; });
}

template<>
template<typename A, typename B>
auto bind_t<std::future>::operator()(std::future<A> _a) const{
    return [_a = std::move(_a)]
        (Func<A, std::future<B>> auto f) mutable{
            return std::async(
                std::launch::deferred,
                [f, _a = std::move(_a)](std::future<A>) mutable{
                    return f(_a.get()).get();
                },
                std::move(_a)
            );
        };
}

template<>
struct is_monad<std::future> {
    constexpr static bool value = true;
};

template<>
template<typename A>
std::optional<A> pure_t<std::optional>::operator()(A a) const {
    return {a};
}

template<>
template<typename A, typename B>
auto bind_t<std::optional>::operator()(std::optional<A> _a) const{
    return [_a](Func<A, std::optional<B>> auto f) {
        if(_a)
            return f(_a.value());
        else return std::optional<B>{};
    };
}

template<>
struct is_monad<std::optional> {
    constexpr static bool value = true;
};

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

    constexpr auto o = pure<std::optional>(1);
    constexpr auto oListF = _fmap<std::optional, int, const char *>(f);
    constexpr auto oo = oListF(o);
    std::cout << oo.value() << std::endl;
}
