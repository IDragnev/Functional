# Functional
## Useful higher-order functions and function objects for easier functional programming in C++  
  ### constexpr version of [std::invoke](https://en.cppreference.com/w/cpp/utility/functional/invoke)   
  ### [curry](https://en.wikipedia.org/wiki/Currying) any function: 
   ```C++
    auto sum = [](auto x, auto y, auto z) { return x + y + z; };  
    auto curriedSum = curry(sum);  
    
    CHECK(curriedSum(1, 2, 3) == 6);
    CHECK(curriedSum(1, 2)(3) == 6);
    CHECK(curriedSum(1)(2, 3) == 6);
    CHECK(curriedSum(1)(2)(3) == 6);
   ```
  ### compose any number of functions:  
  ```C++
  auto toString = [](auto num) { return std::to_string(num); };

  // f = plus789 * plus456 * toString * id
  auto f = compose(plus("789"s), plus("456"s), toString, id);
  
  CHECK(f(123) == "123456789"s);
  ```
  ### superpose any number of functions:  
  ```C++
class Item
{
public:
  //...
    int id() const { /* ... */ }
};

auto itemSaver = [](auto id, const auto& filename) { /* ... */ };
auto correspondingFile = [](const Item* item) -> std::string { /* ... */ };
auto save = superpose(itemSaver, &Item::id, correspondingFile);

std::vector<Item*> items;
//...
std::for_each(std::cbegin(items), std::cend(items), save);
```
  ### combine existing predicates:  
```C++
 auto isPositive = [](auto x) { return x > 0; };
 auto isEven = [](auto x) { return x % 2 == 0; };

 const auto nums = std::vector<int>{ -1, -2, 0, 1, 2, 3, 4 };

 auto pos = std::find_if(std::cbegin(nums),
                         std::cend(nums),
                         allOf(isPositive, isEven));

 CHECK(pos == std::cbegin(nums) + 4);
 
 pos = std::find_if(std::cbegin(nums),
                    std::cend(nums),
                    anyOf(isPositive, isEven));

 CHECK(pos == std::cbegin(nums) + 1);
  ```
  ### inverse existing predicates:
  ```C++
  auto isPositive = [](auto x) { return x > 0; };
  const int nums[] = { 1, 2, 0, -1, 2 };

  auto it = std::find_if(std::begin(nums), 
                         std::end(nums), 
                         inverse(isPositive));

  CHECK(it == nums + 2);
  ```
  ### flip binary functions:  
  ```C++
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
  ```
  ### matching items by specific criteria:
  ```C++
  class Person
  {
  public:
  //...
    unsigned id() const;
    const std::string& name() const;
  };
  
  std::vector<Person*> people;
  //...
  auto targetId = someFunction();
  auto matchesTargetId = matches(targetId, &Person::id);
  auto pos = std::find_if(std::cbegin(people),
                          std::cend(people),
                          matchesTargetId);
  //...
  auto targetName = otherFunction();
  auto matchesTargetName = matches(std::move(targetName), &Person::name);
  pos = std::find_if(std::cbegin(people),
                     std::cend(people),
                     matchesTargetName);
  ```
### and more. Examples can be found in the [tests](https://github.com/IDragnev/Functional/blob/master/Functional/tests.cpp).
