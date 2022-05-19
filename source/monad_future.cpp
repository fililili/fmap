#include <functional>
#include <future>
#include <iostream>
#include <type_traits>

template <typename F, typename A, typename B>
concept Func = std::is_invocable_r<B, F, A>::value;

template<typename A>
constexpr std::future<A> pure(A a) {
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

template<
    template <typename>typename Monad,
    typename A,
    typename B
>
constexpr Func<Monad<A>, Monad<B>> auto _fmap(Func<A,B> auto f) {
    return [f](Monad<A> fa) {
        return bind<A, B>(std::move(fa))
            ([f](A a){
                return pure(f(a));
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
