#pragma once

#include "invoke.hpp"

namespace IDragnev::Functional
{
    namespace Detail
    {
        template <typename Callable, typename... Args>
        auto curryImpl(Callable f, Args&&... args)
        {
            return[f, ...boundArgs = std::forward<decltype(args)>(args)](auto&&... rest) -> decltype(auto)
            {
                if constexpr (std::is_invocable_v<decltype(f), decltype(boundArgs)..., decltype(rest)...>)
                {
                    return (invoke)(f, boundArgs..., std::forward<decltype(rest)>(rest)...);
                }
                else
                {
                    return Detail::curryImpl(f, std::move(boundArgs)..., std::forward<decltype(rest)>(rest)...);
                }
            };
        }
    }

    inline const auto curry = [](auto f) { return Detail::curryImpl(f); };
}