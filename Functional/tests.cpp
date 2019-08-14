#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "Functional.h"
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>
#include <array>
#include <forward_list>

using namespace std::string_literals;
using namespace IDragnev::Functional;

TEST_CASE("testing invoke")
{
	SUBCASE("with member function")
	{
		struct X
		{
			int f() const { return 1; }
		} x;

		CHECK(invoke(&X::f, &x) == 1);
	}

	SUBCASE("with member pointer")
	{
		struct X
		{
			int member = 10;
		} x;

		CHECK(invoke(&X::member, &x) == 10);
	}
	
	SUBCASE("with a regular function")
	{
		auto f = [](auto x) { return x + 1; };

		CHECK(invoke(f, 10) == 11);
	}

	SUBCASE("with reference wrapper")
	{
		struct HeavyObject { } x;
		auto f = [](const HeavyObject&) { return true; };

		CHECK((invoke)(f, std::cref(x)));
	}

	SUBCASE("computing at compile time")
	{
		constexpr auto isPositive = [](auto x) constexpr { return x > 0; };

		static_assert(invoke(isPositive, 10));
	}
}

TEST_CASE("testing Identity")
{
	auto id = Identity{};

	SUBCASE("basics")
	{
		CHECK(id(1) == 1);
		CHECK(id(1) != 2);
	}

	SUBCASE("identity uses perfect forwarding")
	{
		std::string str = "s";
		std::string&& ref = id(std::move(str));

		CHECK(ref == str);
	}

	SUBCASE("computing at compile time")
	{
		static_assert(id(1) == 1);
	}
}

TEST_CASE("testing plus")
{
	SUBCASE("plus takes its left operand as an argument")
	{
		using Strings = std::vector<std::string>;
		auto strings = Strings{ "a", "b", "c" };
		const auto expected = Strings{ "a!", "b!", "c!" };

        std::transform(std::begin(strings),
                       std::end(strings),
                       std::begin(strings),
                       plus("!"));
		
		CHECK(strings == expected);
	}

	SUBCASE("plus uses perfect forwarding")
	{
		auto source = "123"s;
		auto f = plus("456");

		auto result = f(std::move(source));

		CHECK(result == "123456"s);
		CHECK(source == ""s);
	}
}

TEST_CASE("testing inverse")
{
	constexpr auto isPositive = [](auto x) constexpr { return x > 0; };

	SUBCASE("basics")
	{
		const int nums[] = { 1, 2, 0, -1, 2 };

        auto it = std::find_if(std::begin(nums), 
                               std::end(nums), 
                               inverse(isPositive));

		CHECK(it == nums + 2);
	}

	SUBCASE("computing at compile time")
	{
		static_assert(inverse(isPositive)(-1));
	}
}

TEST_CASE("testing superposition")
{
	SUBCASE("basics")
	{
		auto f = superpose(std::greater_equal{}, std::multiplies{}, std::plus{});
		auto g = superpose(std::greater_equal{}, std::plus{}, std::multiplies{});

		CHECK(f(2, 3));
		CHECK(!g(2, 3));
	}

	SUBCASE("superposition handles reference return types")
	{
		const auto id = Identity{};
		const auto max = [](const auto& x, const auto& y) -> const auto& { return x >= y ? x : y; };
		const auto x = 1;

		auto g = superpose(max, id, id);

		const auto& result = g(x);

		CHECK(result == x);
	}

	SUBCASE("computing at compile time")
	{
		constexpr auto id = Identity{};
		static_assert(superpose(std::minus{}, id, id)(1) == 0);
	}
}

TEST_CASE("testing composition")
{
	auto id = Identity{};
	
	SUBCASE("basics")
	{
		auto toString = [](auto num) { return std::to_string(num); };

		// f = plus789 * plus456 * toString * id
		auto f = compose(plus("789"s), plus("456"s), toString, id);

		CHECK(f(123) == "123456789"s);
	}

	SUBCASE("composition handles reference return types")
	{
		auto f = compose(id, id, id);
		auto x = 10;

		int&& y = f(std::move(x));
	}

	SUBCASE("the result of compose uses perfect forwarding on its arguments")
	{
		auto f = [](int&& x) -> int&& { return std::move(x); };

		auto g = compose(plus(1), f);

		CHECK(g(1) == 2);
	}

	SUBCASE("computing at compile time")
	{
		constexpr auto plusOne = [](auto x) constexpr { return x + 1; };
		constexpr auto minusOne = [](auto x) constexpr { return x - 1; };
		
		static_assert(compose(plusOne, minusOne)(2) == 2);
	}
}

TEST_CASE("testing equals")
{
	SUBCASE("basics")
	{
		CHECK(equals("123"s)("123"s));
		CHECK(!equals("lhs"s)("rhs"s));
	}

	SUBCASE("equals allows implicit type conversions")
	{
		struct X
		{
			int x = 1;
			operator int() const { return x; }
		} x;

		CHECK(equals(1)(x));
	}
}

TEST_CASE("testing matches")
{
	struct Item
	{
		std::string key = "target";
	} first, second{ "s" };
	
	auto extractKey = [](const auto& x) { return x.key; };
	auto matchesTarget = matches(first.key, extractKey);

	CHECK(matchesTarget(first));
	CHECK(!matchesTarget(second));
}

TEST_CASE("testing curry")
{
	SUBCASE("basics")
	{
		auto sum = [](auto x, auto y, auto z) { return x + y + z; };

		auto curriedSum = curry(sum);

		CHECK(curriedSum(1, 2, 3) == 6);
		CHECK(curriedSum(1, 2)(3) == 6);
		CHECK(curriedSum(1)(2, 3) == 6);
		CHECK(curriedSum(1)(2)(3) == 6);
	}

	SUBCASE("the curried function captures by forwarding"
		    "and then moves all the arguments on invocation")
	{
		auto f = [](int&& x, std::string&& y) { return x; };
		const auto x = 1;

		auto curriedF = curry(f);

		CHECK(curriedF(x)("y"s) == x);
	}

	SUBCASE("passing non-copiable types by reference to the curried function")
	{
		class NonCopiable
		{
		public:
			NonCopiable() = default;
			NonCopiable(const NonCopiable&) = delete;

			int plus(int y, int z) const { return x + y + z; }

		private:
			const int x = 10;
		} nonCopiable;

		auto f = [](const NonCopiable& x, int y, int z) { return x.plus(y, z); };

		auto curriedF = curry(f);
		auto fWithBoundX = curriedF(std::cref(nonCopiable));

		CHECK(fWithBoundX(1, 2) == 13);
	}
}

TEST_CASE("testing flip")
{
	SUBCASE("basics")
	{
		using List = std::forward_list<int>;

		//some existing function we can reuse
		auto insertBack = [](auto x, auto&& container) -> decltype(auto)
		{
			container.push_front(x);
			return std::move(container);
		};
		auto nums = { 1, 2, 3, 4, 5 };

        auto reversedNums = std::accumulate(std::cbegin(nums),
                                            std::cend(nums),
                                            List{},
                                            flip(insertBack));

		CHECK(reversedNums == List{ 5, 4, 3, 2, 1 });
	}

	SUBCASE("computing at compile time")
	{
		static_assert(flip(std::less{})(2, 1));
	}
}

TEST_CASE("testing allOf and anyOf")
{
	constexpr auto isPositive = [](auto x) constexpr { return x > 0; };
	constexpr auto isEven = [](auto x) constexpr { return x % 2 == 0; };

	const auto nums = std::vector<int>{ -1, -2, 0, 1, 2, 3, 4 };

	SUBCASE("allOf basics")
	{

        auto pos = std::find_if(std::cbegin(nums),
                                std::cend(nums),
                                allOf(isPositive, isEven));

		CHECK(pos == std::cbegin(nums) + 4);
	}

	SUBCASE("anyOf basics")
	{
        auto pos = std::find_if(std::cbegin(nums),
                                std::cend(nums),
                                anyOf(isPositive, isEven));

		CHECK(pos == std::cbegin(nums) + 1);
	}

	SUBCASE("allOf can compute at compile time")
	{
		static_assert(allOf(isPositive, isEven)(2));
	}

	SUBCASE("anyOf can compute at compile time")
	{
		static_assert(anyOf(isPositive, isEven)(-2));
	}
}

TEST_CASE("testing bindFirst")
{
    auto f = bindFirst(std::plus{}, 1);

    CHECK(f(2) == 3);
}