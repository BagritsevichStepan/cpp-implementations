#include <iostream>
#include "cpp_signal.h"
#include <cassert>

int main() {
    cpp::signal::Signal<void()> signal{};

    uint32_t got1 = 0;
    auto conn1 = signal.Connect([&] { ++got1; });

    uint32_t got2 = 0;
    auto conn2 = signal.Connect([&] { ++got2; });

    signal();

    assert(1 == got1);
    assert(1 == got2);
    std::cout << "Got1: " << got1 << " Got2: " << got2 << std::endl;

    signal();

    assert(2 == got1);
    assert(2 == got2);
    std::cout << "Got1: " << got1 << " Got2: " << got2 << std::endl;

    return 0;
}
