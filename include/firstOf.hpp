#include <type_traits>
#include "invoke.hpp"

namespace IDragnev::Functional
{
    namespace Detail
    {
        struct DeletedT { };

        template <typename... Fs>
        class FirstOf { };

        template <typename... Fs>
        FirstOf(Fs...) -> FirstOf<Fs...>;

        template <typename F, typename... Fs>
        class FirstOf<F, Fs...>
        {
        private:
            using Tail = FirstOf<Fs...>;

        public:
            constexpr FirstOf(F first, Fs... tail)
                : first(std::move(first)),
                  tail(std::move(tail)...)
            {
            }

            template <typename... Args,
                      bool FirstOverloadMatched = std::is_invocable_v<const F&, Args...>,
                      typename MatchedOverload = const std::conditional_t<FirstOverloadMatched, F, Tail>&,
                      typename InvokeResult = std::invoke_result_t<MatchedOverload, Args...>,
                      typename = std::enable_if_t<!std::is_same_v<InvokeResult, DeletedT>>
            > constexpr InvokeResult operator()(Args&&... args) const
                noexcept(std::is_nothrow_invocable_v<MatchedOverload, Args...>)
            {
                if constexpr (FirstOverloadMatched) {
                    return (invoke)(first, std::forward<decltype(args)>(args)...);
                }
                else {
                    return (invoke)(tail, std::forward<decltype(args)>(args)...);
                }
            }

            template <typename... Args,
                      typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<const F&, Args...>, DeletedT>>
            > constexpr void operator()(Args&&...) const = delete;

        private:
            [[no_unique_address]] F first;
            [[no_unique_address]] Tail tail;
        };
    } // namespace Detail

    template <typename F>
    class Deleted
    {
    public:
        constexpr Deleted(F f) : f(std::move(f)) { }

        template <typename... Args,
                  typename = std::enable_if_t<std::is_invocable_v<const F&, Args...>>>
        constexpr auto operator()(Args &&...) -> Detail::DeletedT;

    private:
        [[no_unique_address]] F f;
    };

    inline constexpr auto firstOf = [](auto f, auto... fs)
    {
        return Detail::FirstOf{f, fs...};
    };
} // namespace IDragnev::Functional