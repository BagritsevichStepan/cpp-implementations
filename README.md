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
Function

### Member functions
| Function | Description |
| --- | --- |
| `operator->` | List all *new or modified* files |
| `operator bool` | Show file differences that **haven't been** staged |
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
Signal

### Member functions
| Function | Description |
| --- | --- |
| `operator->` | List all *new or modified* files |
| `operator bool` | Show file differences that **haven't been** staged |
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
Intrusive List

### Member functions
| Function | Description |
| --- | --- |
| `operator->` | List all *new or modified* files |
| `operator bool` | Show file differences that **haven't been** staged |
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
Shared Pointer. Weak Pointer

### Member functions
| Function | Description |
| --- | --- |
| `operator->` | List all *new or modified* files |
| `operator bool` | Show file differences that **haven't been** staged |
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
