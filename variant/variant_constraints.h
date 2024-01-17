#ifndef CPP_IMPLEMENTATIONS_VARIANT_CONSTRAINTS_H
#define CPP_IMPLEMENTATIONS_VARIANT_CONSTRAINTS_H

#include <cstdint>
#include <type_traits>
#include <utility>

namespace cpp::variant {

    template <typename... Ts>
    class Variant;


    struct InPlace {
        explicit InPlace() = default;
    };

    inline constexpr InPlace kInPlace{};

    template <typename T>
    struct InPlaceType {
        explicit InPlaceType() = default;
    };

    template <typename T>
    inline constexpr InPlaceType<T> kInPlaceType{};

    template <std::size_t I>
    struct InPlaceIndex {
        explicit InPlaceIndex() = default;
    };

    template <std::size_t I>
    inline constexpr InPlaceIndex<I> kInPlaceIndex{};


    namespace details {

        template <typename T>
        struct IsInPlaceType {
            static constexpr bool value = false;
        };

        template <typename U>
        struct IsInPlaceType<InPlaceType<U>> {
            static constexpr bool value = true;
        };

        template <size_t N>
        struct IsInPlaceType<InPlaceIndex<N>> {
            static constexpr bool value = true;
        };

        template <typename T>
        constexpr bool kIsInPlaceTypeValue = IsInPlaceType<T>::value;


        template <typename... Ts>
        concept IsCopyConstructible = (std::is_copy_constructible_v<Ts> && ...);

        template <typename... Ts>
        concept IsTriviallyCopyConstructible = IsCopyConstructible<Ts...> && (std::is_trivially_copy_constructible_v<Ts> && ...);

        template <typename... Ts>
        concept IsMoveConstructible = (std::is_move_constructible_v<Ts> && ...);

        template <typename... Ts>
        concept IsTriviallyMoveConstructible = IsMoveConstructible<Ts...> && (std::is_trivially_move_constructible_v<Ts> && ...);

        template <typename... Ts>
        concept IsCopyAssignable = IsCopyConstructible<Ts...> && (std::is_copy_assignable_v<Ts> && ...);

        template <typename... Ts>
        concept IsTriviallyDestructible = (std::is_trivially_destructible_v<Ts> && ...);

        template <typename... Ts>
        concept IsTriviallyCopyAssignable = IsCopyAssignable<Ts...> && IsTriviallyCopyConstructible<Ts...>
                && (std::is_trivially_copy_assignable_v<Ts> && ...) && IsTriviallyDestructible<Ts...>;

        template <typename... Ts>
        concept IsMoveAssignable = IsMoveConstructible<Ts...> && (std::is_move_assignable_v<Ts> && ...);

        template <typename... Ts>
        concept IsTriviallyMoveAssignable = IsMoveAssignable<Ts...> && IsTriviallyMoveConstructible<Ts...>
                && (std::is_trivially_move_assignable_v<Ts> && ...) && IsTriviallyDestructible<Ts...>;

        template <typename Variant, typename T, typename ChosenT, typename... Ts>
        concept IsConvertibleToChosenTypeVariant = (sizeof...(Ts) > 0) && (!kIsInPlaceTypeValue < std::remove_cvref_t<T>>)
                && !std::is_same_v<T, Variant> && std::is_constructible_v<ChosenT, T>;

        template<typename T, typename T_I>
        concept IsValidConversion = requires(T &&x) {
            T_I{std::forward<T>(x)};
        };

    } // End of namespace cpp::variant::details

} // End of namespace cpp::variant

#endif //CPP_IMPLEMENTATIONS_VARIANT_CONSTRAINTS_H
