#ifndef __FUNCTIONAL_H_INCLUDED__
#define __FUNCTIONAL_H_INCLUDED__

#include <type_traits>

namespace IDragnev::Functional
{	
	struct LessThan
	{
		template <typename T, typename U>
		constexpr auto operator()(const T& lhs, const U& rhs) const noexcept(noexcept(lhs < rhs))
		{
			return lhs < rhs;
		}
	};

	struct GreaterThan
	{
		template <typename T, typename U>
		constexpr auto operator()(const T& lhs, const U& rhs) const noexcept(noexcept(lhs > rhs))
		{
			return lhs > rhs;
		}
	};

	struct EqualTo
	{
		template <typename T, typename U>
		constexpr auto operator()(const T& lhs, const U& rhs) const noexcept(noexcept(lhs == rhs))
		{
			return lhs == rhs;
		}
	};

	struct Identity
	{
		template <typename T>
		constexpr decltype(auto) operator()(T&& item) const noexcept
		{
			return std::forward<T>(item);
		}
	};

	struct EmptyFunction
	{
		template <typename... Args>
		constexpr void operator()(Args&&...) const noexcept { }
	};

	namespace Detail
	{
		template <typename... Ts>
		inline constexpr bool areNothrowCopyConstructible = (std::is_nothrow_copy_constructible_v<Ts> && ...);
	}

	template <typename F>
	F superpose(F&&) = delete;

	template <typename F, typename... Gs>
	constexpr inline 
	auto superpose(F f, Gs... funs) noexcept(Detail::areNothrowCopyConstructible<F, Gs...>)
	{
		return [f, funs...](const auto&... args) constexpr mutable -> decltype(auto)
		{
			return f(funs(args...)...);
		};
	}

	template <typename F, typename G>
	constexpr inline 
	auto compose(F f, G g) noexcept(noexcept(superpose(f, g)))
	{
		return superpose(f, g);
	}

	template <typename F, typename G, typename... Gs>
	constexpr auto compose(F f, G g, Gs... funs) noexcept(noexcept(compose(f, g)))
	{
		return compose(compose(f, g), funs...);
	}

	template <typename T>
	inline auto equalTo(T key) noexcept(std::is_nothrow_move_constructible_v<T>)
	{
		return [lhs = std::move(key)](const auto& rhs)
		{ 
			return lhs == rhs;
		};
	}

	template <typename Key, typename KeyExtractor>
	inline auto matches(Key key, KeyExtractor extractKey) 
	{
		return compose(equalTo(std::move(key)), std::move(extractKey));
	}

	template <typename T>
	inline auto plus(T rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
	{
		return[rhs = std::move(rhs)](auto&& lhs) -> decltype(auto)
		{
			using F = decltype(lhs);
			return std::forward<F>(lhs) + rhs;
		};
	}

	template <typename Predicate>
	constexpr inline 
	auto inverse(Predicate p) noexcept(std::is_nothrow_copy_constructible_v<Predicate>)
	{
		return [p](auto&&... args) constexpr noexcept(std::is_nothrow_invocable_v<decltype(p), decltype(args)...>)
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
			return [f, boundArgs = std::move(t)](auto&&... rest) -> decltype(auto)
			{
				auto newArgs = std::make_tuple(std::forward<decltype(rest)>(rest)...);
				auto args = std::tuple_cat(std::move(boundArgs), std::move(newArgs));

				using ArgsTuple = decltype(std::tuple_cat(std::move(boundArgs), std::move(newArgs)));
				
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

	template <typename BinaryFunction>
	constexpr auto flip(BinaryFunction f) noexcept
	{
		return[f](auto&& x, auto&& y) constexpr noexcept(std::is_invocable_v<decltype(f), decltype(y), decltype(x)>)
		-> decltype(auto)
		{
			using X = decltype(x);
			using Y = decltype(y);
			return f(std::forward<Y>(y), std::forward<X>(x));
		};
	}

	namespace Detail
	{
		constexpr auto andAll = [](auto... args) constexpr noexcept
		{
			return (args && ...);
		};

		constexpr auto orAll = [](auto... args) constexpr noexcept
		{
			return (args || ...);
		};
		
		template <typename CombinationOp, typename... Predicates> 
		constexpr auto 
		combinePredicatesWith(CombinationOp op, Predicates... predicates) noexcept(areNothrowCopyConstructible<Predicates...>)
		{
			return [op, predicates...](const auto&... args) constexpr noexcept(andAll(std::is_nothrow_invocable_v<decltype(predicates), decltype(args)...>...))
			{
				return op(predicates(args...)...);
			};
		}
	}

	template <typename... Predicates>
	constexpr inline
	auto allOf(Predicates... predicates) noexcept(noexcept(Detail::combinePredicatesWith(Detail::andAll, predicates...)))
	{
		using Detail::andAll;
		using Detail::combinePredicatesWith;

		return combinePredicatesWith(andAll, predicates...);
	}

	template <typename... Predicates>
	constexpr inline
	auto anyOf(Predicates... predicates) noexcept(noexcept(Detail::combinePredicatesWith(Detail::orAll, predicates...)))
	{
		using Detail::orAll;
		using Detail::combinePredicatesWith;

		return combinePredicatesWith(orAll, predicates...);
	}
}

#endif //__FUNCTIONAL_H_INCLUDED__