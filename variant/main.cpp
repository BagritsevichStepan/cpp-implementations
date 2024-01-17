#include <iostream>
#include <cassert>
#include "variant.h"

namespace {
    constexpr void SimpleTest() {
        cpp::variant::Variant<int, double> variant{42.0};
        assert(variant.Index() == 1);
        variant = 5;
        assert(variant.Index() == 0);
    }
}

int main() {
    SimpleTest();
    return 0;
}
