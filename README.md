# Functional
## Useful higher-order functions and function objects for easier functional programming in C++17, most of which can be used at compile time.   

## You can:       
  ### use a constexpr version of [std::invoke](https://en.cppreference.com/w/cpp/utility/functional/invoke)   
  ### [curry](https://en.wikipedia.org/wiki/Currying) any function: 
   ```C++
    //some existing function with many arguments which
    //we might regularly call with the same file/format or both
    void printTo(File& file, Format f, Item item);
   
    //so we can curry it and avoid making numerous specific overloads:
    const auto curriedPrintTo = curry(printTo);
    
    const auto log = curriedPrintTo(std::ref(debugFile));
    //...
    log(FileFormat::regular, someItem);
   
    //and then we notice that even log is
    //mostly used with a regular format
    const auto regularLog = log(Format::regular); //equivalent to curriedPrintTo(std::ref(debugFile), Format::regular);
    
    //now we can do it either way:
    regularLog(someItem); 
    log(FileFormat::regular, someItem);
    log(FileFormat::compact, someItem);
    curriedPrintTo(std::ref(debugFile), FileFormat::regular, someItem);    
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
  ### combine any number of existing predicates:  
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
  auto insertFront = [](auto x, auto&& container) -> decltype(auto)
  {
     container.push_front(x);
     return std::move(container);
  };
  auto nums = { 1, 2, 3, 4, 5 };

  auto reversedNums = std::accumulate(std::cbegin(nums),
                                      std::cend(nums),
                                      List{},
                                      flip(insertFront));

  CHECK(reversedNums == List{ 5, 4, 3, 2, 1 });
  ```
  ### match items by specific criteria:
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
  
  ### write more expressive code:
  Instead of
  ```C++
  std::find_if(std::cbegin(nums), std::cend(nums), [](auto x) { return x < 0; });
  std::transform(std::cbegin(nums), 
                 std::cend(nums), 
                 std::begin(result),
                 [](auto x) { return x * 5; });
  ```
  you can simply write
  ```C++ 
  std::find_if(std::cbegin(nums), std::cend(nums), lessThan(0));
  std::transform(std::cbegin(nums), 
                 std::cend(nums), 
                 std::begin(result),
                 times(5));
  ```
  There are also the corresponding functions for -, +, %, /, ==, >, >=, <=, !=.
  They all use perfect forwarding and the bound argument is on the right as in Haskell's (==0), (*5) etc.
### and more. Examples can be found in the [tests](https://github.com/IDragnev/Functional/blob/master/Functional/tests.cpp).
