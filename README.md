# CPP Implementations
Implementations of variant, optional, function and other std classes.

+ [Optional](#optional)
+ [Variant](#variant)
+ [Function](#function)
+ [Signal](#signal)
+ [Intrusive List](#list)
+ [Shared Pointer. Weak Pointer](#ptr)

## Optional
Optional implementations is fully `constexpr`. The copy ctor, move ctor, copy assign and move assign are also supported if the stored type supports them.

### Member functions
| Function | Description |
| --- | --- |
| `operator->` | List all *new or modified* files |
| `operator bool` | Show file differences that **haven't been** staged |
### Example
```cpp
constexpr bool Test() {
  std::array<int, 2> arr = {5, -1};
  cpp::optional::Optional<decltype(arr)> opt{arr};
  return opt->at(1) == -1 && opt->at(0) == 5;
}

static_assert(Test());
```

## Variant
The interface and all properties and guarantees correspond to `std::variant` (specialization for `std::hash` is not implemented). Variant retains triviality for special members (destructors, constructors, and assignment operators).

### Member functions
| Function | Description |
| --- | --- |
| `operator->` | List all *new or modified* files |
| `operator bool` | Show file differences that **haven't been** staged |
### Example
```cpp
constexpr void Test() {
  cpp::variant::Variant<int, double> variant{42.0};
  assert(variant.Index() == 1);
  variant = 5;
  assert(variant.Index() == 0);
}
```

## Function
Implementation of [`std::function`](https://en.cppreference.com/w/cpp/utility/functional/function). Instances of `cpp::function::Function` can store, copy, and invoke any **callable** target -- functions, lambda expressions or other function objects.

### Member functions
| Function | Description |
| --- | --- |
| `F operator()(Args... args)` | Invokes the target with specified arguments |
| `T* target() noexcept` | Obtains a pointer to the stored target |
| `explicit operator bool() const noexcept` | Checks if a target is contained |
| `void Swap(Function& other)` | Swaps the contents with `other` |

### Non-member functions
| Function | Description |
| --- | --- |
| `void swap(Function<F(Args...)>& a, Function<F(Args...)>& b)` | Exchanges the given functions |

### Example
```cpp
int state = 5;
auto print_num = [&state](int i) -> int {
  return state + static_cast<int>(i);
};

cpp::function::Function<int(int)> fun{std::move(print_num)};
assert(fun(5) == 10);
assert(fun(9) == 14);
```

## Signal
Implementation of signals similar to [those used in Qt](https://doc.qt.io/qt-5/signalsandslots.html).

You can add callbacks to the signal, which will be called when an event has occurred. After connecting to the signal, the `Connection` class will be returned to you, which you can use to disconnect the callback from the signal.

**The main problem** is that in the body of a some callback the user can disconnect other callbacks from the signal. This can be solved by tricky method using two [intrusive lists](#list).

### Member functions
Signal:
| Function | Description |
| --- | --- |
| `Connection Connect(std::function<void(Args...)> slot)` | Connects function to the signal |
| `void operator()(Args... args)` | Invokes all connected callbacks |

Connection:
| Function | Description |
| --- | --- |
| `void Disconnect()` | Disconnects function from the signal |

### Example
```cpp
cpp::signal::Signal<void()> signal{};

uint32_t got1 = 0;
auto conn1 = signal.Connect([&] { ++got1; });
uint32_t got2 = 0;
auto conn2 = signal.Connect([&] { ++got2; });

signal();
assert(1 == got1);
assert(1 == got2);
signal();
assert(2 == got1);
assert(2 == got2);
```

## <a name="list"></a>Intrusive List
Intrusive implementations of the [`std::list`](https://en.cppreference.com/w/cpp/container/list).
More about intrusive data structures: [Link](https://www.boost.org/doc/libs/1_82_0/doc/html/intrusive/intrusive_vs_nontrusive.html).

First, you need to create custom node that contains your data. The class of your node must inherit `cpp::intrusive::ListElement<NodeTag>`. You can also pass your custom tag to limit the node types that will be stored in the list.

After that, you can add nodes to the `cpp::intrusive::List<YourCustomNode, YourCustomNodeTag>`
### Member types
| Function | Description |
| --- | --- |
| `iterator` | **Bidirectional iterator** to list node |
| `const_iterator` | **Bidirectional iterator** to `const` list node |
| `reverse_iterator` | `std::reverse_iterator<iterator>` |
| `const_reverse_iterator` | `std::reverse_iterator<const_iterator>` |

### Member functions
| Function | Description |
| --- | --- |
| `T* Front() const noexcept` | Access the first element |
| `T* Back() const noexcept` | Access the last element |
| `iterator Insert(const_iterator position, T& element)` | Inserts elements |
| `iterator Erase(const_iterator position)` | Erases elements |
| `void PushBack(T& element)` | Adds `element` to the end |
| `void PushFront(T& element)` | Inserts `element` to the beginning |
| `void PopBack()` | Removes the last element |
| `void PopFront()` | Removes the first element |
| `void Swap(List& other) noexcept` | Swaps the contents with `other` |
| `iterator begin()`<br>`iterator end()`<br>`const_iterator cbegin()`<br>`const_iterator cend()` | Returns (const) iterator to the beginning / end |
| `reverse_iterator rbegin()`<br>`reverse_iterator rend()`<br>`const_reverse_iterator crbegin()`<br>`const_reverse_iterator crend()` | Returns reverse (const) iterator to the beginning / end |

### Non-member functions
| Function | Description |
| --- | --- |
| `std::ostream &operator<<(std::ostream& os, const List& list)` | Displays the list and all its nodes |

### Example
Node:
```cpp
class NodeTag;

struct Node : public cpp::intrusive::ListElement<NodeTag> {
public:
    explicit Node(int value) : value_(value) {}

    friend std::ostream& operator<<(std::ostream& os, const Node& node) {
        return os << "Node=[value=" << node.value_ << "]";
    }

    int value_;
};
```
List:
```cpp
void Test() {
    cpp::intrusive::List<Node, NodeTag> list;

    Node node1 = Node{1};
    Node node2 = Node{2};
    Node node3 = Node{3};
    Node node4 = Node{4};
    Node node5 = Node{5};

    list.PushBack(node1);
    list.PushFront(node2);
    list.PushBack(node3);
    list.PushFront(node4);
    list.PushBack(node5);

    std::cout << list << std::endl;
}
```
Output:
```
List=[Node=[value=4], Node=[value=2], Node=[value=1], Node=[value=3], Node=[value=5]]
```


## <a name="ptr"></a>Shared Pointer. Weak Pointer
Implementation of [`std::shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr) and [`std::weak_ptr`](https://en.cppreference.com/w/cpp/memory/weak_ptr).
The [`std::make_shared`](https://en.cppreference.com/w/cpp/memory/shared_ptr/make_shared) was also implemented.

### Member functions
Shared Pointer:
| Function | Description |
| --- | --- |
| `T* Get() const noexcept` | Returns the stored pointer |
| `T& operator*() const noexcept` | Dereferences the stored pointer |
| `T* operator->() const noexcept` | Dereferences the stored pointer |
| `size_t UseCount() const noexcept` | Returns the number of `SharedPointer` objects referring<br>to the same managed object |
| `void Reset()` | Replaces the managed object |
| `void Reset(T* data, Deleter&& deleter)` | Replaces the managed object with an object pointed to by `data` |
| `void Swap(SharedPointer<T>& other) noexcept` | Swaps the managed objects with `other` |

Weak Pointer:
| Function | Description |
| --- | --- |
| `explicit WeakPointer(const SharedPointer<T>& shared_pointer)` | Constructs new `WeakPointer` which shares an object<br>managed by `shared_pointer` |
| `size_t UseCount() const noexcept` | Returns the number of `SharedPointer` objects referring<br>to the same managed object |
| `bool IsExpired() const noexcept` | Checks whether the referenced object was already deleted |
| `void Swap(WeakPointer& other) noexcept` | Swaps the managed objects with `other` |

### Non-member functions
| Function | Description |
| --- | --- |
| `SharedPointer<T> MakeShared(Args&&... args)` | Creates a shared pointer that manages a new object |

### Example
Shared Pointer:
```cpp
cpp::pointer::SharedPointer<EmptyClass> shared_ptr1{new EmptyClass};
auto shared_ptr2 = shared_ptr1;
auto shared_ptr3 = shared_ptr2;

assert(shared_ptr1.UseCount() == 3);
assert(shared_ptr2.UseCount() == 3);
assert(shared_ptr3.UseCount() == 3);
```
Weak Pointer:
```cpp
cpp::pointer::WeakPointer<EmptyClass>* weak_pointer1;
{
  cpp::pointer::SharedPointer<EmptyClass> shared_ptr_for_weak = cpp::pointer::MakeShared<EmptyClass>();
  weak_pointer1 = new cpp::pointer::WeakPointer(shared_ptr_for_weak);
}
assert(weak_pointer1->IsExpired());
assert(weak_pointer1->UseCount() == 0);
```
