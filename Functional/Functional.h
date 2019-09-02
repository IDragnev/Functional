#ifndef __FUNCTIONAL_H_INCLUDED__
#define __FUNCTIONAL_H_INCLUDED__

#include "invoke.h"
#include <utility>

namespace IDragnev::Functional
{
    inline constexpr auto identity = [](auto&& x) constexpr noexcept -> decltype(auto)
    {
        return std::forward<decltype(x)>(x);
    };

    inline constexpr auto emptyFunction = [](auto&&...) constexpr noexcept { };

    namespace Detail
    {
        inline constexpr auto andAll = [](auto... args) constexpr noexcept
        {
            return (args && ...);
        };

        inline constexpr auto orAll = [](auto... args) constexpr noexcept
        {
            return (args || ...);
        };

        template <typename... Ts>
        inline constexpr bool areNothrowCopyConstructible = andAll(std::is_nothrow_copy_constructible_v<Ts>...);
    }

    template <typename F>
    F superpose(F&&) = delete;

    template <typename F, typename... Gs>
    constexpr inline
    auto superpose(F f, Gs... funs) noexcept(Detail::areNothrowCopyConstructible<F, Gs...>)
    {
        return[f, funs...](const auto&... args) constexpr -> decltype(auto)
        {
            using Detail::andAll;
            static_assert(andAll(std::is_invocable_v<decltype(funs), decltype(args)...>...),
                "Incompatible arguments given to Gs or their signatures are incompatible");
            static_assert(std::is_invocable_v<decltype(f), std::invoke_result_t<decltype(funs), decltype(args)...>...>,
                "F and Gs have incompatible signatures");

            return (invoke)(f, (invoke)(funs, args...)...);
        };
    }

    template <typename F, typename G>
    constexpr inline
    auto compose(F f, G g) noexcept(Detail::areNothrowCopyConstructible<F, G>)
    {
        return[f, g](auto&&... args) constexpr -> decltype(auto)
        {
            static_assert(std::is_invocable_v<decltype(g), decltype(args)...>,
                          "Incompatible arguments given to G");
            static_assert(std::is_invocable_v<decltype(f), std::invoke_result_t<decltype(g), decltype(args)...>>,
                          "F and G have incompatible signatures");

            return (invoke)(f, (invoke)(g, std::forward<decltype(args)>(args)...));
        };
    }

    template <typename F, typename G, typename... Gs>
    constexpr auto compose(F f, G g, Gs... funs) noexcept(noexcept(compose(f, g)))
    {
        return compose(compose(f, g), funs...);
    }

    template <typename Predicate>
    constexpr inline
    auto inverse(Predicate p) noexcept(std::is_nothrow_copy_constructible_v<Predicate>)
    {
        return[p](auto&&... args) constexpr noexcept(std::is_nothrow_invocable_v<decltype(p), decltype(args)...>)
        {
            return !p(std::forward<decltype(args)>(args)...);
        };
    }

    namespace Detail
    {
        template <typename Callable, typename ArgsTuple>
        inline constexpr bool isInvocableWithPackedArgs = false;

        template <typename Callable, typename... Args>
        inline constexpr bool isInvocableWithPackedArgs<Callable, std::tuple<Args...>> = std::is_invocable_v<Callable, Args...>;

        template <typename Callable, typename... Args>
        auto curryImpl(Callable f, std::tuple<Args...>&& t)
        {
            return[f, boundArgs = std::move(t)](auto&&... rest) -> decltype(auto)
            {
                auto newArgs = std::make_tuple(std::forward<decltype(rest)>(rest)...);
                auto args = std::tuple_cat(boundArgs, std::move(newArgs));

                using ArgsTuple = decltype(std::tuple_cat(boundArgs, newArgs));

                if constexpr (isInvocableWithPackedArgs<Callable, ArgsTuple>)
                {
                    return std::apply(f, std::move(args));
                }
                else
                {
                    return Detail::curryImpl(f, std::move(args));
                }
            };
        }
    }

    template <typename Callable>
    inline auto curry(Callable f)
    {
        return Detail::curryImpl(f, {});
    }

    inline constexpr auto flip = [](auto f) noexcept
    {
        return[f](auto&& x, auto&& y) constexpr noexcept(std::is_invocable_v<decltype(f), decltype(y), decltype(x)>)
        -> decltype(auto)
        {
            using X = decltype(x);
            using Y = decltype(y);
            return (invoke)(f, std::forward<Y>(y), std::forward<X>(x));
        };
    };

    namespace Detail
    {
        inline constexpr auto makePredicateCombinator = [](auto op) noexcept(std::is_nothrow_copy_constructible_v<decltype(op)>)
        {
            return[op](auto... predicates) constexpr noexcept(areNothrowCopyConstructible<decltype(op), decltype(predicates)...>)
            {
                return superpose(op, predicates...);
            };
        };
    }

    inline constexpr auto allOf = Detail::makePredicateCombinator(Detail::andAll);
    inline constexpr auto anyOf = Detail::makePredicateCombinator(Detail::orAll);

    inline const auto bindFirst = [](auto f, auto arg) noexcept(std::is_nothrow_copy_constructible_v<decltype(f)> &&
                                                                std::is_nothrow_move_constructible_v<decltype(arg)>)
    {
        return[f, first = std::move(arg)](auto&&... rest)
        {
            static_assert(std::is_invocable_v<decltype(f), decltype(first), decltype(rest)...>,
                          "Incompatible arguments supplied");
            return (invoke)(f, first, std::forward<decltype(rest)>(rest)...);
        };
    };

    namespace Detail
    {
        inline const auto makeBinaryFunctionRightArgumentBinder = compose(curry(bindFirst), flip);
    }

    inline const auto plus = Detail::makeBinaryFunctionRightArgumentBinder(std::plus{});
    inline const auto minus = Detail::makeBinaryFunctionRightArgumentBinder(std::minus{});
    inline const auto times = Detail::makeBinaryFunctionRightArgumentBinder(std::multiplies{});
    inline const auto divided = Detail::makeBinaryFunctionRightArgumentBinder(std::divides{});
    inline const auto mod = Detail::makeBinaryFunctionRightArgumentBinder(std::modulus{});

    inline const auto equals = Detail::makeBinaryFunctionRightArgumentBinder(std::equal_to{});
    inline const auto differs = Detail::makeBinaryFunctionRightArgumentBinder(std::not_equal_to{});
    inline const auto lessThan = Detail::makeBinaryFunctionRightArgumentBinder(std::less{});
    inline const auto greaterThan = Detail::makeBinaryFunctionRightArgumentBinder(std::greater{});
    inline const auto greaterOrEqualTo = Detail::makeBinaryFunctionRightArgumentBinder(std::greater_equal{});
    inline const auto lessOrEqualTo = Detail::makeBinaryFunctionRightArgumentBinder(std::less_equal{});

    inline const auto matches = [](auto key, auto keyExtractor)
    {
        return compose(equals(std::move(key)), keyExtractor);
    };
}

#endif //__FUNCTIONAL_H_INCLUDED__