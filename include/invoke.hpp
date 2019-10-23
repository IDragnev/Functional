#pragma once

#include <type_traits>

namespace IDragnev::Functional
{
    namespace Detail
    {
        template <typename T>
        struct IsReferenceWrapperT : std::false_type { };

        template <typename T>
        struct IsReferenceWrapperT<std::reference_wrapper<T>> : std::true_type { };

        template <typename T>
        inline constexpr bool isReferenceWrapper = IsReferenceWrapperT<T>::value;

        template <typename Ret,
                  typename C,
                  typename T,
                  typename... Args
        > constexpr decltype(auto)
        invokeMemberFunction(Ret C::*f, T&& obj, Args&&... args)
        {
            if constexpr (std::is_base_of_v<C, std::decay_t<T>>) {
                return (obj.*f)(std::forward<Args>(args)...);
            }
            else if constexpr (isReferenceWrapper<std::decay_t<T>>) {
                return (obj.get().*f)(std::forward<Args>(args)...);
            }
            else { //pointer
                return ((*obj).*f)(std::forward<Args>(args)...);
            }
        }

        template <typename MemberT,
                  typename C,
                  typename T
        > constexpr decltype(auto) invokeMemberPointer(MemberT C::*f, T&& obj)
        {
            if constexpr (std::is_base_of_v<C, std::decay_t<T>>) {
                return obj.*f;
            }
            else if constexpr (isReferenceWrapper<std::decay_t<T>>) {
                return obj.get().*f;
            }
            else { //pointer
                return (*obj).*f;
            }
        }
    } //namespace Detail

    template <typename F, typename... Args>
    constexpr std::invoke_result_t<F, Args...>
    invoke(F&& f, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>)
    {
        if constexpr (std::is_member_function_pointer_v<std::remove_reference_t<F>>) {
            return Detail::invokeMemberFunction(std::forward<F>(f),
                                                std::forward<Args>(args)...);
        }
        else if constexpr (std::is_member_object_pointer_v<std::remove_reference_t<F>> &&
                           sizeof...(Args) == 1) {
            return Detail::invokeMemberPointer(std::forward<F>(f),
                                               std::forward<Args>(args)...);
        }
        else {
            return std::forward<F>(f)(std::forward<Args>(args)...);
        }
    }
} //namespace IDragnev::Functional