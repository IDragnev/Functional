#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "include/functional.hpp"
#include <algorithm>
#include <functional>
#include <numeric>
#include <string>
#include <vector>
#include <array>
#include <forward_list>

using namespace std::string_literals;
using namespace IDragnev::Functional;

TEST_CASE("invoke")
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
        const auto f = [](auto x) { return x + 1; };

        CHECK(invoke(f, 10) == 11);
    }

    SUBCASE("with reference wrapper")
    {
        struct HeavyObject { } x;
        const auto f = [](const HeavyObject&) { return true; };

        CHECK((invoke)(f, std::cref(x)));
    }

    SUBCASE("computing at compile time")
    {
        constexpr auto isPositive = [](auto x) constexpr { return x > 0; };

        static_assert(invoke(isPositive, 10));
    }
}

TEST_CASE("identity")
{
    SUBCASE("basics")
    {
        CHECK(identity(1) == 1);
        CHECK(identity(1) != 2);
    }

    SUBCASE("identity uses perfect forwarding")
    {
        std::string str = "s";
        std::string&& ref = identity(std::move(str));

        CHECK(ref == str);
    }

    SUBCASE("computing at compile time")
    {
        static_assert(identity(1) == 1);
    }
}

TEST_CASE("plus")
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
        const auto f = plus("456");

        auto result = f(std::move(source));

        CHECK(result == "123456"s);
        CHECK(source == ""s);
    }
}

TEST_CASE("minus")
{
    SUBCASE("minus takes its left operand as an argument")
    {
        const auto nums = { 1, 2, 3 };
        const auto expected = std::vector<int>{ 0, 1, 2 };
        auto result = std::vector<int>(nums.size(), 0);

        std::transform(std::begin(nums),
                       std::end(nums),
                       std::begin(result),
                       minus(1));

        CHECK(result == expected);
    }

    SUBCASE("minus uses perfect forwarding")
    {
        struct X
        {
            int operator-(int) && { return 1; }
        };

        const auto f = minus(1);

        const auto result = f(X{});

        CHECK(result == 1);
    }
}

TEST_CASE("inverse")
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

TEST_CASE("superposition")
{
    SUBCASE("basics")
    {
        const auto f = superpose(std::greater_equal{}, std::multiplies{}, std::plus{});
        const auto g = superpose(std::greater_equal{}, std::plus{}, std::multiplies{});

        CHECK(f(2, 3));
        CHECK(!g(2, 3));
    }

    SUBCASE("superposition handles reference return types")
    {
        const auto max = [](const auto& x, const auto& y) -> const auto& { return x >= y ? x : y; };
        const auto x = 1;

        const auto g = superpose(max, identity, identity);

        const auto& result = g(x);

        CHECK(result == x);
    }

    SUBCASE("computing at compile time")
    {
        static_assert(superpose(std::minus{}, identity, identity)(1) == 0);
    }
}

TEST_CASE("composition")
{
    SUBCASE("basics")
    {
        const auto toString = [](auto num) { return std::to_string(num); };

        // f = plus789 * plus456 * toString * identity
        const auto f = compose(plus("789"s), plus("456"s), toString, identity);

        CHECK(f(123) == "123456789"s);
    }

    SUBCASE("composition handles reference return types")
    {
        const auto f = compose(identity, identity, identity);
        auto x = 10;

        int&& y = f(std::move(x));
    }

    SUBCASE("the result of compose uses perfect forwarding on its arguments")
    {
        const auto f = [](int&& x) -> int&& { return std::move(x); };

        const auto g = compose(plus(1), f);

        CHECK(g(1) == 2);
    }

    SUBCASE("computing at compile time")
    {
        constexpr auto plusOne = [](auto x) constexpr { return x + 1; };
        constexpr auto minusOne = [](auto x) constexpr { return x - 1; };

        static_assert(compose(plusOne, minusOne)(2) == 2);
    }
}

TEST_CASE("equals")
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

TEST_CASE("differs")
{
    SUBCASE("basics")
    {
        CHECK(differs("123"s)("122"s));
        CHECK(!differs("abc"s)("abc"s));
    }

    SUBCASE("differs allows implicit type conversions")
    {
        struct X
        {
            int x = 1;
            operator int() const { return x; }
        } x;

        CHECK(differs(2)(x));
    }
}

TEST_CASE("matches")
{
    struct Item
    {
        std::string key = "target";
    } first, second{ "s" };

    const auto extractKey = [](const auto& x) { return x.key; };
    const auto matchesTarget = matches(first.key, extractKey);

    CHECK(matchesTarget(first));
    CHECK(!matchesTarget(second));
}

TEST_CASE("curry")
{
    SUBCASE("basics")
    {
        const auto sum = [](auto x, auto y, auto z) { return x + y + z; };

        const auto curriedSum = curry(sum);

        CHECK(curriedSum(1, 2, 3) == 6);
        CHECK(curriedSum(1, 2)(3) == 6);
        CHECK(curriedSum(1)(2, 3) == 6);
        CHECK(curriedSum(1)(2)(3) == 6);
    }

    SUBCASE("the curried function captures by forwarding,"
            "copies the bound arguments and forwards the new arguments on invocation")
    {
        const auto f = [](int x, std::string&& y) { return x; };
        const auto x = 1;

        const auto curriedF = curry(f);

        CHECK(curriedF(x)("y"s) == x);
    }

    SUBCASE("multiple invocations")
    {
        using Strings = std::vector<std::string>;

        const auto f = [](std::string&& str, std::uint32_t n)
        {
            return str + std::to_string(n);
        };
        const auto format = curry(f)("~");
        const auto nums = { 1, 2, 3 };
        auto result = Strings(nums.size());

        std::transform(std::cbegin(nums),
                       std::cend(nums),
                       std::begin(result),
                       format);

        CHECK(result == Strings{ "~1", "~2", "~3" });
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

        const auto f = [](const NonCopiable& x, int y, int z) { return x.plus(y, z); };

        const auto curriedF = curry(f);
        const auto fWithBoundX = curriedF(std::cref(nonCopiable));

        CHECK(fWithBoundX(1, 2) == 13);
    }
}

TEST_CASE("flip")
{
    SUBCASE("basics")
    {
        using List = std::forward_list<int>;

        //some existing function we can reuse
        const auto insertFront = [](auto x, auto&& container) 
        {
            container.push_front(x);
            return std::move(container);
        };
        const auto nums = List{ 1, 2, 3, 4, 5 };
        const auto reversedNums = List{ 5, 4, 3, 2, 1 };

        const auto result = std::accumulate(std::cbegin(nums),
                                            std::cend(nums),
                                            List{},
                                            flip(insertFront));

        CHECK(result == reversedNums);
    }

    SUBCASE("computing at compile time")
    {
        static_assert(flip(std::less{})(2, 1));
    }
}

TEST_CASE("allOf and anyOf")
{
    constexpr auto isPositive = [](auto x) constexpr { return x > 0; };
    constexpr auto isEven = [](auto x) constexpr { return x % 2 == 0; };

    const auto nums = std::vector<int>{ -1, -2, 0, 1, 2, 3, 4 };

    SUBCASE("allOf basics")
    {

        const auto pos = std::find_if(std::cbegin(nums),
                                      std::cend(nums),
                                      allOf(isPositive, isEven));

        CHECK(pos == std::cbegin(nums) + 4);
    }

    SUBCASE("anyOf basics")
    {
        const auto pos = std::find_if(std::cbegin(nums),
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

TEST_CASE("bindFront")
{
    SUBCASE("basics")
    {
        const auto f = bindFront(std::plus{}, 1);

        CHECK(f(2) == 3);
    }

    SUBCASE("multiple invocations")
    {
        const auto sum = [](auto x, auto y, auto z)
        {
            return x + y + z;
        };

        const auto sumWith3 = bindFront(sum, 1, 2);
        const auto nums = std::vector{ 1, 2, 3 };
        auto result = std::vector<int>(nums.size());

        std::transform(std::cbegin(nums),
                       std::cend(nums),
                       std::begin(result),
                       sumWith3);

        CHECK(result == std::vector{4, 5, 6});
    }
}

TEST_CASE("firstOf")
{
    struct First { };
    struct Second { };
    struct Third { };

    SUBCASE("basics")
    {
        constexpr auto f = firstOf(
            [](First) { return 1; },
            [](Second) { return 2; },
            [](Third) { return 3; }
        );

        static_assert(f(First{}) == 1);
        static_assert(f(Second{}) == 2);
        static_assert(f(Third{}) == 3);
    }

    SUBCASE("deleting an overload")
    {
        constexpr auto f = firstOf(
            [](First) { return 1; },
            Deleted([](Second) { return 2; }),
            [](Third) { return 3; }
        );
        using F = decltype(f);
        
        static_assert(!std::is_invocable_v<F, Second>);
        static_assert(f(First{}) == 1);
        static_assert(f(Third{}) == 3);
    }
    
    SUBCASE("implicit conversions are dangerous!")
    {
        constexpr auto f = firstOf(
            [](double) { return 1; },
            Deleted([](int) { return 2; }),
            [](Third) { return 3; }
        );
        using F = decltype(f);
        
        static_assert(std::is_invocable_v<F, int>);
        static_assert(f(1) == 1);
        static_assert(f(Third{}) == 3);
    }
}