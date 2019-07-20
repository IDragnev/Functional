#include "CppUnitTest.h"
#include "Functional.h"
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>
#include <array>
#include <forward_list>

using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IDragnev::Functional;

namespace FunctionalTest
{		
	auto sum = [](auto x, auto y) { return x + y; };
	auto multiply = [](auto x, auto y) { return x * y; };
	auto toString = [](auto num) { return std::to_string(num); };

	TEST_CLASS(FunctionalTest)
	{
	public:
		TEST_METHOD(lessThan)
		{
			Assert::IsTrue(LessThan{}(1, 2));
			Assert::IsFalse(LessThan{}(1, 1));
			Assert::IsFalse(LessThan{}(2, 1));
		}

		TEST_METHOD(functorEqualTo)
		{
			Assert::IsTrue(EqualTo{}(1, 1));
			Assert::IsFalse(EqualTo{}(1, 2));
		}

		TEST_METHOD(identity)
		{
			auto id = Identity{};

			Assert::AreEqual(1, id(1));
			Assert::AreNotEqual(2, id(1));
		}

		TEST_METHOD(identityUsesPerfectForwarding)
		{
			std::string str = "s";
			std::string&& ref = Identity{}(std::move(str));

			Assert::AreSame(ref, str);
		}

		TEST_METHOD(lessThanEqualToAndIdentityCanComputeAtCompileTime)
		{
			using TrueType = std::bool_constant<LessThan{}(1, 2)>;
			using FalseType = std::bool_constant<EqualTo{}(1, 2)>;
			using ArrayOfSizeOne = std::array<int, Identity{}(1)>;
		}

		TEST_METHOD(plusTakesLeftOperand)
		{
			using Strings = std::vector<std::string>;
			auto strings = Strings{ "a", "b", "c" };
			const auto expected = Strings{ "a!", "b!", "c!" };

			std::transform(std::begin(strings), std::end(strings), std::begin(strings), plus("!"));

			Assert::IsTrue(strings == expected);
		}

		TEST_METHOD(plusForwardsItsArgument)
		{
			auto source = "123"s;
			
			auto result = plus("456")(std::move(source));

			Assert::IsTrue(result == "123456"s);
			Assert::IsTrue(source == ""s);
		}

		TEST_METHOD(inversingAPredicate)
		{
			auto predicate = [](int x) { return x >= 0; };
			auto nums = { 1, 2, -1, 2 };

			auto it = std::find_if(std::begin(nums), std::end(nums), inverse(predicate));

			Assert::IsNotNull(it);
			Assert::AreEqual(-1, *it);
		}

		TEST_METHOD(inverseCanComputAtCompileTime)
		{
			constexpr auto isZero = [](auto x) constexpr { return x == 0; };
			static_assert(inverse(isZero)(1));
		}

		TEST_METHOD(superposition)
		{
			auto f = [](auto x, auto y) { return x >= y; };
			auto g = superpose(f, multiply, sum);
			auto h = superpose(f, sum, multiply);

			Assert::IsTrue(g(2, 3));
			Assert::IsFalse(h(2, 3));
		}

		TEST_METHOD(composition)
		{
			auto f = compose(plus("789"s), plus("456"s), toString, Identity{});

			Assert::AreEqual(f(123), "123456789"s);
		}

		TEST_METHOD(superpositionHandlesReferenceReturnTypes)
		{
			auto id = Identity{};
			testHandlesReferenceReturnTypes(superpose(id, id));
		}

		TEST_METHOD(compositionHandlesReferenceReturnTypes)
		{
			auto id = Identity{};
			testHandlesReferenceReturnTypes(compose(id, id));
		}

	private:
		template <typename Callable>
		void testHandlesReferenceReturnTypes(Callable f)
		{
			auto source = "123"s;
			auto copy = source;

			Assert::AreSame(source, f(source));
			Assert::AreNotSame(source, f(copy));
		}

	public:
		TEST_METHOD(superpositionAndCompositionCanComputeAtCompileTime)
		{
			constexpr auto plusOne = [](auto x) constexpr { return x + 1; };
			constexpr auto minusOne = [](auto x) constexpr { return x - 1; };
			constexpr auto areEqual = [](auto x, auto y) constexpr { return x == y; };

			using FalseType = std::bool_constant<superpose(areEqual, plusOne, minusOne)(4)>;
			using ArrayOfSizeOne = std::array<int, compose(minusOne, plusOne)(1)>;
		}

		TEST_METHOD(higherOrderEqualTo)
		{
			Assert::IsTrue(equalTo("123"s)("123"s));
			Assert::IsFalse(equalTo("lhs"s)("rhs"s));
		}

		TEST_METHOD(higherOrderEqualToAllowsImplicitTypeConversions)
		{
			struct X
			{
				int x = 1;
				operator int() const { return x; }
			} x;

			Assert::IsTrue(equalTo(1)(x));
		}

		TEST_METHOD(matchingByKey)
		{
			struct Item
			{
				std::string key = "target";
			} first, second{ "s" };
			auto extractKey = [](const auto& x) { return x.key; };
			auto matchesTarget = matches(first.key, extractKey);

			Assert::IsTrue(matchesTarget(first));
			Assert::IsFalse(matchesTarget(second));
		}

		TEST_METHOD(curryingAFunction)
		{
			auto f = [](auto x, auto y, auto z) { return x + y + z; };
			
			auto curriedF = curry(f);		

			Assert::AreEqual(curriedF(1, 2, 3), 6);
			Assert::AreEqual(curriedF(1, 2)(3), 6);
			Assert::AreEqual(curriedF(1)(2, 3), 6);
			Assert::AreEqual(curriedF(1)(2)(3), 6);
		}

		TEST_METHOD(theCurriedFunctionCapturesByForwardingAndThenMovesAllTheArgumentsOnInvocation)
		{
			auto f = [](int&& x, std::string&& y) { return x; };
			const auto x = 1;

			auto curriedF = curry(f);

			Assert::AreEqual(curriedF(x)("y"s), x);
		}

		TEST_METHOD(passingNonCopiableTypesByRefToTheCurriedFunction)
		{
			struct NonCopiable
			{
				int sum(int y, int z) const { return x + y + z; }

				const int x = 10;
			} nonCopiable;
			
			auto f = [](const NonCopiable& x, int y, int z) { return x.sum(y, z); };

			auto curriedF = curry(f);
			auto fWithBoundX = curriedF(std::cref(nonCopiable));

			Assert::AreEqual(fWithBoundX(1, 2), 13);
		}

		TEST_METHOD(flippingABinaryFunction)
		{
			//some existing function we can reuse
			auto insertBack = [](auto x, auto&& container) -> decltype(auto)
			{	
				container.push_front(x);
			    return std::move(container); 
			};
			auto nums = { 1, 2, 3, 4, 5 };

			auto reversedNums = std::accumulate(std::cbegin(nums), 
				                                std::cend(nums), 
				                                std::forward_list<int>{}, 
				                                flip(insertBack));

			Assert::IsTrue(reversedNums == std::forward_list<int>{5, 4, 3, 2, 1});
		}

		TEST_METHOD(flipCanComputeAtCompileTime)
		{
			static_assert(flip(LessThan{})(2, 1));
		}
	};
}