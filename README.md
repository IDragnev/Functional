# Functional
## Useful higher-order functions and function objects for easier functional programming in C++20, most of which can be used at compile time.   

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
  const auto toString = [](auto num) { return std::to_string(num); };

  // f = plus789 * plus456 * toString * id
  const auto f = compose(plus("789"s), plus("456"s), toString, id);
  
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

const auto itemSaver = [](auto id, const auto& filename) { /* ... */ };
const auto correspondingFile = [](const Item* item) -> std::string { /* ... */ };
const auto save = superpose(itemSaver, &Item::id, correspondingFile);

std::vector<Item*> items;
//...
std::for_each(std::cbegin(items), std::cend(items), save);
```
  ### combine any number of existing predicates:  
```C++
 const auto isPositive = [](auto x) { return x > 0; };
 const auto isEven = [](auto x) { return x % 2 == 0; };

 const auto nums = std::vector<int>{ -1, -2, 0, 1, 2, 3, 4 };

 const auto pos = std::find_if(std::cbegin(nums),
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
  const auto isPositive = [](auto x) { return x > 0; };
  const int nums[] = { 1, 2, 0, -1, 2 };

  const auto it = std::find_if(std::cbegin(nums), 
                               std::cend(nums), 
                               inverse(isPositive));

  CHECK(it == nums + 2);
  ```
  ### flip binary functions:  
  ```C++
  using List = std::forward_list<int>;

  //some existing function we can reuse
  const auto insertFront = [](auto x, auto&& container)
  {
     container.push_front(x);
     return std::move(container);
  };
  const auto nums = List{ 1, 2, 3, 4, 5 };

  const auto reversedNums = std::accumulate(std::cbegin(nums),
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
  const auto targetId = someFunction();
  const auto matchesTargetId = matches(targetId, &Person::id);
  const auto pos = std::find_if(std::cbegin(people),
                                std::cend(people),
                                matchesTargetId);
  //...
  const auto targetName = otherFunction();
  const auto matchesTargetName = matches(std::move(targetName), &Person::name);
  pos = std::find_if(std::cbegin(people),
                     std::cend(people),
                     matchesTargetName);
  ```
  

  ### build overload sets on the fly:
  ```C++
  const auto f = firstOf(
    [](int x) { /*...*/ },
    [](const std::string& x) { /*...*/ },
    [](SomeClass) { /*...*/ }
  );

  const auto g = firstOf(
    [](const std::string& x) { /*...*/ },
    Deleted([](int x) { /*...*/ }),   //propagates deletion for int
    [](SomeClass) { /*...*/ }
  );
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
### and more. Examples can be found in the [tests](https://github.com/IDragnev/Functional/blob/master/tests/functional.cpp).