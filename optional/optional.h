#ifndef CPP_IMPLEMENTATIONS_OPTIONAL_H
#define CPP_IMPLEMENTATIONS_OPTIONAL_H

#include <utility>
#include <algorithm>
#include <type_traits>

namespace cpp::optional {

    struct nullopt_t {};
    constexpr nullopt_t nullopt{};

    struct in_place_t {};
    constexpr in_place_t in_place{};

    namespace details {

        struct Dummy {};
        constexpr Dummy dummy{};

        template <typename T>
        union OptionalValue {
            constexpr OptionalValue() : dummy_(dummy) {}
            constexpr OptionalValue(const T& value) : value_(value) {}
            constexpr OptionalValue(T&& value) : value_(std::move(value)) {}

            template <typename... Args>
            constexpr explicit OptionalValue(Args&&... args) : value_(std::forward<Args>(args)...) {}

            Dummy dummy_;
            T value_;
        };

        template <typename T, bool IsTriviallyDestructible>
        class Base {
        public:
            constexpr Base() noexcept : is_present_(false), optional_value_() {}
            constexpr Base(nullopt_t) noexcept : is_present_(false), optional_value_() {}
            constexpr Base(const T& value) : is_present_(true), optional_value_(value) {}
            constexpr Base(T&& value) : is_present_(true), optional_value_(std::move(value)) {}

            template <typename... Args>
            constexpr explicit Base(in_place_t, Args&&... args)
                    : is_present_(true), optional_value_(std::forward<Args>(args)...) {}

            ~Base() { Reset(); }

            void Reset() noexcept {
                if (this->is_present_) {
                    this->value_.~T();
                    this->is_present_ = false;
                }
            }

        protected:
            bool is_present_;
            OptionalValue<T> optional_value_;
        };

        template <typename T>
        class Base<T, true> {
        public:
            constexpr Base() noexcept : is_present_(false), optional_value_() {}
            constexpr Base(nullopt_t) noexcept : is_present_(false), optional_value_() {}
            constexpr Base(const T& value) : is_present_(true), optional_value_(value) {}
            constexpr Base(T&& value) : is_present_(true), optional_value_(std::move(value)) {}

            template <typename... Args>
            constexpr explicit Base(in_place_t, Args&&... args)
                    : is_present_(true), optional_value_(std::forward<Args>(args)...) {}

            constexpr ~Base() {};

            constexpr void Reset() noexcept {
                is_present_ = false;
            }

        protected:
            bool is_present_;
            OptionalValue<T> optional_value_;
        };


        template <typename T, bool IsTriviallyCopyable>
        class CopyMoveBase : public details::Base<T, std::is_trivially_destructible_v<T>> {
        public:
            using Base = details::Base<T, std::is_trivially_destructible_v<T>>;
            using Base::Base;

            constexpr CopyMoveBase() = default;

            constexpr CopyMoveBase(const CopyMoveBase& other) {
                if (other.is_present_) {
                    new (&optional_value_.value_) T(other.optional_value_.value_);
                }
                is_present_ = other.is_present_;
            }

            constexpr CopyMoveBase(CopyMoveBase&& other) noexcept {
                if (other.is_present_) {
                    new (&optional_value_.value_) T(std::move(other.optional_value_.value_));
                }
                is_present_ = other.is_present_;
            }

            constexpr CopyMoveBase& operator=(const CopyMoveBase& other) {
                if (this == &other || !is_present_ && !other.is_present_) {
                    return *this;
                }

                if (is_present_ && !other.is_present_) {
                    this->Reset();
                } else if (!is_present_ && other.is_present_) {
                    new (&optional_value_.value_) T(other.optional_value_.value_);
                } else {
                    optional_value_.value_ = other.optional_value_.value_;
                }

                is_present_ = other.is_present_;

                return *this;
            }

            constexpr CopyMoveBase& operator=(CopyMoveBase&& other) noexcept {
                if (this == &other || !is_present_ && !other.is_present_) {
                    return *this;
                }

                if (is_present_ && !other.is_present_) {
                    this->Reset();
                } else if (!is_present_ && other.is_present_) {
                    new (&optional_value_.value_) T(std::move(other.optional_value_.value_));
                } else {
                    optional_value_.value_ = std::move(other.optional_value_.value_);
                }

                is_present_ = other.is_present_;

                return *this;
            }

        protected:
            using Base::is_present_;
            using Base::optional_value_;
        };

        template <bool Enable>
        struct EnableCopyCtor {};

        template <>
        struct EnableCopyCtor<false> {
            constexpr EnableCopyCtor() = default;
            constexpr EnableCopyCtor(const EnableCopyCtor&) = delete;
            constexpr EnableCopyCtor(EnableCopyCtor&&) noexcept = default;
            constexpr EnableCopyCtor& operator=(const EnableCopyCtor&) = default;
            constexpr EnableCopyCtor& operator=(EnableCopyCtor&&) noexcept = default;
        };

        template <bool Enable>
        struct EnableMoveCtor {};

        template <>
        struct EnableMoveCtor<false> {
            constexpr EnableMoveCtor() = default;
            constexpr EnableMoveCtor(const EnableMoveCtor&) = default;
            constexpr EnableMoveCtor(EnableMoveCtor&&) noexcept = delete;
            constexpr EnableMoveCtor& operator=(const EnableMoveCtor&) = default;
            constexpr EnableMoveCtor& operator=(EnableMoveCtor&&) noexcept = default;
        };

        template <bool Enable>
        struct EnableCopyAssign {};

        template <>
        struct EnableCopyAssign<false> {
            constexpr EnableCopyAssign() = default;
            constexpr EnableCopyAssign(const EnableCopyAssign&) = default;
            constexpr EnableCopyAssign(EnableCopyAssign&&) noexcept = default;
            constexpr EnableCopyAssign& operator=(const EnableCopyAssign&) = delete;
            constexpr EnableCopyAssign& operator=(EnableCopyAssign&&) noexcept = default;
        };

        template <bool Enable>
        struct EnableMoveAssign {};

        template <>
        struct EnableMoveAssign<false> {
            constexpr EnableMoveAssign() = default;
            constexpr EnableMoveAssign(const EnableMoveAssign&) = default;
            constexpr EnableMoveAssign(EnableMoveAssign&&) noexcept = default;
            constexpr EnableMoveAssign& operator=(const EnableMoveAssign&) = default;
            constexpr EnableMoveAssign& operator=(EnableMoveAssign&&) noexcept = delete;
        };

    }

    template <typename T>
    class Optional final : private details::CopyMoveBase<T, std::is_trivially_copyable_v<T>>,
                           details::EnableCopyCtor<std::is_copy_constructible_v<T>>,
                           details::EnableMoveCtor<std::is_move_constructible_v<T>>,
                           details::EnableCopyAssign<std::is_copy_assignable_v<T>>,
                           details::EnableMoveAssign<std::is_move_assignable_v<T>> {
    private:
        using Base = details::CopyMoveBase<T, std::is_trivially_copyable_v<T>>;
        using Base::Base;

    public:
        using Base::Reset;

        constexpr Optional() noexcept : Base() {}
        constexpr Optional(nullopt_t nullopt_) noexcept : Base(nullopt_) {}
        constexpr Optional(const T& value) : Base(value) {}
        constexpr Optional(T&& value) : Base(std::move(value)) {}

        template <typename... Args>
        constexpr explicit Optional(in_place_t in_place_, Args&&... args) : Base(in_place_, std::forward<Args>(args)...) {}

        Optional(const Optional& other) = default;
        Optional(Optional&& other) noexcept = default;
        Optional& operator=(const Optional& other) = default;
        Optional& operator=(Optional&& other) noexcept = default;

        constexpr void Swap(Optional& other);

        template <typename... Args>
        T& Emplace(Args&&... args);

        constexpr T* operator->();
        constexpr const T* operator->() const;

        constexpr T& operator*();
        constexpr const T& operator*() const;

        constexpr explicit operator bool() const;

        template <typename T_, typename U_>
        friend constexpr bool operator==(const Optional<T_>& a, const Optional<U_>& b);

        template <typename T_, typename U_>
        friend constexpr bool operator!=(const Optional<T_>& a, const Optional<U_>& b);

        template <typename T_, typename U_>
        friend constexpr bool operator<(const Optional<T_>& a, const Optional<U_>& b);

        template <typename T_, typename U_>
        friend constexpr bool operator>(const Optional<T_>& a, const Optional<U_>& b);

        template <typename T_, typename U_>
        friend constexpr bool operator<=(const Optional<T_>& a, const Optional<U_>& b);

        template <typename T_, typename U_>
        friend constexpr bool operator>=(const Optional<T_>& a, const Optional<U_>& b);
    };

    template <typename T>
    constexpr void swap(Optional<T>& a, Optional<T>& b);


    // Implementation
    template <typename T>
    constexpr void Optional<T>::Swap(Optional<T>& other) {
        using std::swap;
        swap(this->optional_value_, other.optional_value_);
        swap(this->is_present_, other.is_present_);
    }

    template <typename T>
    template <typename... Args>
    T& Optional<T>::Emplace(Args&&... args) {
        Reset();
        new (&(this->optional_value_.value_)) T(std::forward<Args>(args)...);
        this->is_present_ = true;
    }

    template <typename T>
    constexpr T* Optional<T>::operator->() {
        return &(this->optional_value_.value_);
    }

    template <typename T>
    constexpr const T*  Optional<T>::operator->() const {
        return &(this->optional_value_.value_);
    }

    template <typename T>
    constexpr T& Optional<T>::operator*() {
        return this->optional_value_.value_;
    }

    template <typename T>
    constexpr const T& Optional<T>::operator*() const {
        return this->optional_value_.value_;
    }

    template <typename T>
    constexpr Optional<T>::operator bool() const {
        return this->is_present_;
    }

    template <typename T, typename U>
    constexpr bool operator==(const Optional<T>& a, const Optional<U>& b) {
        return (a && b) ? *a == *b : a.is_present_ == b.is_present_;
    }

    template <typename T, typename U>
    constexpr bool operator!=(const Optional<T>& a, const Optional<U>& b) {
        return (a && b) ? *a != *b : a.is_present != b.is_present_;
    }

    template <typename T, typename U>
    constexpr bool operator<(const Optional<T>& a, const Optional<U>& b) {
        if (!b) {
            return false;
        }
        if (!a) {
            return true;
        }
        return *a < *b;
    }

    template <typename T, typename U>
    constexpr bool operator>(const Optional<T>& a, const Optional<U>& b) {
        if (!a) {
            return false;
        }
        if (!b) {
            return true;
        }
        return *a > *b;
    }

    template <typename T, typename U>
    constexpr bool operator<=(const Optional<T>& a, const Optional<U>& b) {
        if (!a) {
            return true;
        }
        if (!b) {
            return false;
        }
        return *a <= *b;
    }

    template <typename T, typename U>
    constexpr bool operator>=(const Optional<T>& a, const Optional<U>& b) {
        if (!b) {
            return true;
        }
        if (!a) {
            return false;
        }
        return *a >= *b;
    }

    template <typename T>
    constexpr void swap(Optional<T>& a, Optional<T>& b) {
        a.Swap(b);
    }

}

#endif //CPP_IMPLEMENTATIONS_OPTIONAL_H
