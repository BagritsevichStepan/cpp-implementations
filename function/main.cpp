#include <iostream>
#include "function.h"

int main() {
    int state = 5;

    auto print_num = [&state](size_t i) -> int {
        return state + static_cast<int>(i);
    };

    cpp::function::Function<int(size_t)> fun{std::move(print_num)};
    std::cout << fun(5) << std::endl;
    std::cout << fun(9) << std::endl;

    cpp::function::Function<int(size_t)> fun2{std::move(fun)};
    std::cout << fun2(10) << std::endl;

    cpp::function::Function<int(size_t)> fun3 = std::move(fun2);
    std::cout << fun3(30) << std::endl;


    auto print_num2 = [&state](size_t i) -> int {
        return state + static_cast<int>(i) + 3;
    };
    cpp::function::Function<int(size_t)> fun4{std::move(print_num2)};

    std::cout << "Fun4: " << fun4(2) << std::endl;
    std::cout << "Fun3: " << fun3(2) << std::endl;

    using std::swap;
    swap(fun3, fun4);

    std::cout << "Fun4: " << fun4(2) << std::endl;
    std::cout << "Fun3: " << fun3(2) << std::endl;

    if (fun4) {
        std::cout << "Fun4: Yes" << std::endl;
    } else {
        std::cout << "Fun4: No" << std::endl;
    }

    cpp::function::Function<int(size_t)> fun_empty{};
    if (fun_empty) {
        std::cout << "Fun_empty: Yes" << std::endl;
    } else {
        std::cout << "Fun_empty: No" << std::endl;
    }

    swap(fun4, fun_empty);

    if (fun4) {
        std::cout << "Fun4: Yes" << std::endl;
    } else {
        std::cout << "Fun4: No" << std::endl;
    }

    if (fun_empty) {
        std::cout << "Fun_empty: Yes" << std::endl;
    } else {
        std::cout << "Fun_empty: No" << std::endl;
    }


    std::function<int(size_t)> some_func;

    return 0;
}
