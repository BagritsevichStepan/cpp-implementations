#include <iostream>
#include <cassert>
#include "optional.h"

namespace {

    constexpr bool NotPresentDefault() {
        cpp::optional::Optional<bool> opt{};
        return !opt;
    }

    constexpr cpp::optional::Optional<int> InitializeNullopt() {
        return cpp::optional::nullopt;
    }

    constexpr bool NotPresentNullopt() {
        return !InitializeNullopt();
    }

    constexpr bool IsPresent() {
        bool value = false;
        cpp::optional::Optional<bool> opt{value};
        return bool(opt);
    }

    constexpr bool IsPresentRValueReference() {
        cpp::optional::Optional<std::array<int, 1>> opt{{22}};
        return bool(opt);
    }

    constexpr bool IsFive() {
        cpp::optional::Optional<int> opt{5};
        return *opt == 5;
    }

    constexpr bool AssertReferenceOperator() {
        std::array<int, 2> arr = {5, -1};
        cpp::optional::Optional<decltype(arr)> opt{arr};
        return opt->at(1) == -1 && opt->at(0) == 5;
    }

    constexpr bool AssertSwap() {
        std::array<int, 3> arr = {5, -1, 2};

        cpp::optional::Optional<decltype(arr)> opt1{arr};
        cpp::optional::Optional<decltype(arr)> opt2{};

        bool first_assert = opt1->at(2) == 2 && !opt2;

        using std::swap;
        swap(opt1, opt2);

        bool second_assert = !opt1 && opt2->at(1) == -1 && opt2->at(0) == 5;
        return first_assert && second_assert;
    }


    class NonCopyableClass {
        NonCopyableClass() = default;
        NonCopyableClass(const NonCopyableClass&) = delete;
        NonCopyableClass(NonCopyableClass&&) noexcept = delete;
        NonCopyableClass& operator=(const NonCopyableClass&) = delete;
        NonCopyableClass& operator=(NonCopyableClass&&) noexcept = delete;
    };

    void AssertCopyMoveSemantic() {
        using OptionalInt = cpp::optional::Optional<int>;

        assert(std::is_copy_constructible_v<OptionalInt>);
        assert(std::is_move_constructible_v<OptionalInt>);
        assert(std::is_copy_assignable_v<OptionalInt>);
        assert(std::is_move_assignable_v<OptionalInt>);

        OptionalInt opt1{5};
        assert(*opt1 == 5);

        cpp::optional::Optional<int> opt2{opt1};
        assert(*opt2 == 5);

        cpp::optional::Optional<int> opt3{10};
        opt3 = opt1;
        assert(*opt3 == 5);

        using OptionalNonCopyClass = cpp::optional::Optional<NonCopyableClass>;
        assert(!std::is_copy_constructible_v<OptionalNonCopyClass>);
        assert(!std::is_move_constructible_v<OptionalNonCopyClass>);
        assert(!std::is_copy_assignable_v<OptionalNonCopyClass>);
        assert(!std::is_move_assignable_v<OptionalNonCopyClass>);
    }

}

int main() {
    static_assert(NotPresentDefault());
    static_assert(NotPresentNullopt());

    static_assert(IsPresent());
    static_assert(IsPresentRValueReference());

    static_assert(IsFive());
    static_assert(AssertReferenceOperator());
    static_assert(AssertSwap());

    AssertCopyMoveSemantic();
    return 0;
}
