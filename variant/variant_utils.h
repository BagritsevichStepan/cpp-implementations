#ifndef CPP_IMPLEMENTATIONS_VARIANT_UTILS_H
#define CPP_IMPLEMENTATIONS_VARIANT_UTILS_H

#include <cstdint>
#include <exception>
#include <type_traits>
#include <functional>
#include <utility>
#include "variant_constraints.h"

namespace cpp::variant {

    template <typename... Ts>
    class Variant;

    class BadVariantAccess : std::exception {
    public:
        BadVariantAccess() noexcept = default;

        explicit BadVariantAccess(const char* message) noexcept : message_(message) {}

        [[nodiscard]] const char* what() const noexcept override {
            return message_;
        }

    private:
        const char* message_ = "Bad variant access";
    };

    inline constexpr size_t kVariantNPos = -1;


    namespace details {

        template <size_t N, typename... Rest>
        struct NthType {
            using Type = void;
        };

        template <typename T0, typename... Rest>
        struct NthType<0, T0, Rest...> {
            using Type = T0;
        };

        template <size_t N, typename T0, typename... Rest>
        struct NthType<N, T0, Rest...> {
            using Type = typename NthType<N - 1, Rest...>::Type;
        };

        template <size_t N, typename... Ts>
        using At = typename NthType<N, Ts...>::Type;

    } // End of namespace cpp::variant::details


    template <typename T>
    struct VariantSize;

    template <typename... Ts>
    struct VariantSize<Variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

    template <typename... Ts>
    struct VariantSize<const Variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

    template <typename... Ts>
    struct VariantSize<volatile Variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

    template <typename... Ts>
    struct VariantSize<const volatile Variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

    template <typename T>
    inline constexpr size_t kVariantSizeValue = VariantSize<T>::value;


    template <size_t N, typename T>
    struct VariantAlternative;

    template <size_t N, typename... Ts>
    struct VariantAlternative<N, Variant<Ts...>> {
        using Type = details::At<N, Ts...>;
    };

    template <size_t N, typename... Ts>
    struct VariantAlternative<N, const Variant<Ts...>> {
        using Type = const details::At<N, Ts...>;
    };

    template <size_t N, typename... Ts>
    struct VariantAlternative<N, volatile Variant<Ts...>> {
        using Type = volatile details::At<N, Ts...>;
    };

    template <size_t N, typename... Ts>
    struct VariantAlternative<N, const volatile Variant<Ts...>> {
        using Type = const volatile details::At<N, Ts...>;
    };

    template <size_t N, typename T>
    using VariantAlternativeType = typename VariantAlternative<N, T>::Type;


    namespace details {

        template <size_t N, typename T, typename... Ts>
        struct OverloadSet;

        template <size_t N, typename T>
        struct TypeInfo {
            static const size_t index = N;
            using Type = T;
        };

        template <size_t N, typename T, typename U>
        struct OverloadSet<N, T, U> {
            static TypeInfo<N, U> Overload(U) requires IsValidConversion<T, U>;
        };

        template <size_t N, typename T, typename U, typename... Ts>
        struct OverloadSet<N, T, U, Ts...> : OverloadSet<N + 1, T, Ts...> {
            using OverloadSet<N + 1, T, Ts...>::Overload;

            static TypeInfo<N, U> Overload(U) requires IsValidConversion<T, U>;
        };

        template <typename T, typename... Ts>
        using ChosenOverload = decltype(OverloadSet<0, T, Ts...>::Overload(std::declval<T>()));


        template <typename T, typename... Ts>
        struct OccurrencesCount;

        template <typename T>
        struct OccurrencesCount<T> {
            constexpr static const size_t value = 0;
        };

        template <typename T, typename U>
        struct OccurrencesCount<T, U> {
            constexpr static const size_t value = static_cast<size_t>(std::is_same_v<T, U>);
        };

        template <typename T, typename U, typename... Ts>
        struct OccurrencesCount<T, U, Ts...> {
            constexpr static const size_t value = static_cast<size_t>(std::is_same_v<T, U>) + OccurrencesCount<T, Ts...>::value;
        };

        template <typename T, typename... Ts>
        inline constexpr bool kOccursOnlyOnceValue = OccurrencesCount<T, Ts...>::value == 1;


        template <typename T, size_t N, typename... Ts>
        struct FindInPack;

        template <typename T, size_t N>
        struct FindInPack<T, N> {
            static constexpr const size_t value = -1;
        };

        template <typename T, size_t N, typename U, typename... Ts>
        struct FindInPack<T, N, U, Ts...> {
            static constexpr const size_t value = std::is_same_v<T, U> ? N : FindInPack<T, N + 1, Ts...>::value;
        };

        template <typename T, typename... Ts>
        inline constexpr size_t kFindInPackValue = FindInPack<T, 0, Ts...>::value;

    } // End of namespace cpp::variant::details


    template <typename T, typename... Ts>
    constexpr bool kHoldsAlternative(const Variant<Ts...> &v) noexcept requires(details::kOccursOnlyOnceValue<T, Ts...>) {
        return v.index() == details::kFindInPackValue<T, Ts...>;
    }

    template <size_t N, typename... Ts>
    constexpr VariantAlternativeType<N, Variant<Ts...>>& Get(Variant<Ts...>& v) {
        if (N == v.Index()) {
            return v.val.Get(kInPlaceIndex<N>);
        } else {
            throw BadVariantAccess("Variant stores alternative with another index");
        }
    }

    template <size_t N, typename... Ts>
    constexpr const VariantAlternativeType<N, Variant<Ts...>>& Get(const Variant<Ts...>& v) {
        if (N == v.Index()) {
            return v.val.Get(kInPlaceIndex<N>);
        } else {
            throw BadVariantAccess("Variant stores alternative with another index");
        }
    }

    template <size_t N, typename... Ts>
    constexpr VariantAlternativeType<N, Variant<Ts...>>&& Get(Variant<Ts...>&& v) {
        return std::move(Get<N>(v));
    }

    template <size_t N, typename... Ts>
    constexpr const VariantAlternativeType<N, Variant<Ts...>>&& Get(const Variant<Ts...>&& v) {
        return std::move(Get<N>(v));
    }

    template <typename T, typename... Ts, size_t Index = details::kFindInPackValue<T, Ts...>>
    constexpr T& Get(Variant<Ts...>& v) {
        if (kHoldsAlternative<T>(v)) {
            return v.val.Get(kInPlaceIndex<Index>);
        } else {
            throw BadVariantAccess("Variant stores another alternative");
        }
    }

    template <typename T, typename... Ts, size_t Index = details::kFindInPackValue<T, Ts...>>
    constexpr const T& Get(const Variant<Ts...>& v) {
        if (kHoldsAlternative<T>(v)) {
            return v.val.get(kInPlaceIndex<Index>);
        } else {
            throw BadVariantAccess("variant stores another alternative");
        }
    }

    template <typename T, typename... Ts, size_t Index = details::kFindInPackValue<T, Ts...>>
    constexpr T&& Get(Variant<Ts...> && v) {
        return std::move(Get<T>(v));
    }

    template <typename T, typename... Ts, size_t Index = details::kFindInPackValue<T, Ts...>>
    constexpr const T&& Get(const Variant<Ts...> &&v) {
        return std::move(Get<T>(v));
    }

    template <size_t N, typename... Ts>
    constexpr std::add_pointer_t<VariantAlternativeType<N, Variant<Ts...>>> GetIf(Variant<Ts...>* pv) noexcept {
        if (pv == nullptr) {
            return nullptr;
        }
        if (N == pv->Index()) {
            return std::addressof(Get<N>(*pv));
        } else {
            return nullptr;
        }
    }

    template <size_t N, typename... Ts>
    constexpr std::add_pointer_t<const VariantAlternativeType<N, Variant<Ts...>>> GetIf(const Variant<Ts...> *pv) noexcept {
        if (pv == nullptr) {
            return nullptr;
        }
        if (N == pv->index()) {
            return std::addressof(pv->val.Get(kInPlaceIndex<N>));
        } else {
            return nullptr;
        }
    }

    template <typename T, typename... Ts, size_t N = details::kFindInPackValue<T, Ts...>>
    constexpr std::add_pointer_t<T> GetIf(Variant<Ts...>* pv) noexcept {
        return GetIf<N>(pv);
    }

    template <typename T, typename... Ts, size_t N = details::kFindInPackValue<T, Ts...>>
    constexpr std::add_pointer_t<const T> GetIf(const Variant<Ts...>* pv) noexcept {
        return GetIf<N>(pv);
    }


    namespace details {

        template <typename... Ts>
        constexpr std::array<std::common_type_t<Ts...>, sizeof...(Ts)> MakeArray(Ts&&... t) noexcept {
            return {std::forward<Ts>(t)...};
        }

        template <typename F, typename... Vs, size_t... Inds>
        constexpr auto GenFmatrixImpl(std::index_sequence<Inds...>) noexcept {
            return [](F f, Vs... vs) { return std::invoke(std::forward<F>(f), Get<Inds>(std::forward<Vs>(vs))...); };
        }

        template <typename F, size_t... Inds>
        constexpr auto GenFmatrixIndexImpl(std::index_sequence<Inds...>) noexcept {
            return [](F f) { return std::invoke(std::forward<F>(f), (std::integral_constant<size_t, Inds>())...); };
        }

        template <typename F, typename... Vs, size_t... FixedInds, size_t... NextInds, typename... RestInds>
        constexpr auto GenFmatrixImpl(std::index_sequence<FixedInds...>, std::index_sequence<NextInds...>, RestInds... rInds) noexcept {
            return MakeArray(GenFmatrixImpl<F, Vs...>(std::index_sequence<FixedInds..., NextInds>(), rInds...)...);
        };

        template <typename F, size_t... FixedInds, size_t... NextInds, typename... RestInds>
        constexpr auto GenFmatrixIndexImpl(std::index_sequence<FixedInds...>, std::index_sequence<NextInds...>, RestInds... rInds) noexcept {
            return MakeArray(GenFmatrixIndexImpl<F>(std::index_sequence<FixedInds..., NextInds>(), rInds...)...);
        };

        template <typename F, typename... Vs>
        constexpr auto GenFmatrix() noexcept {
            return GenFmatrixImpl<F, Vs...>(std::index_sequence<>(), std::make_index_sequence<kVariantSizeValue<std::remove_reference_t<Vs>>>()...);
        }

        template <typename F, typename... Vs>
        constexpr auto GenFmatrixIndex() noexcept {
            return GenFmatrixIndexImpl<F>(std::index_sequence<>(), std::make_index_sequence<kVariantSizeValue<std::remove_reference_t<Vs>>>()...);
        }

        template <typename T>
        constexpr T& AtImpl(T& matrix) noexcept {
            return matrix;
        }

        template <typename T, typename... Inds>
        constexpr auto& AtImpl(T& matrix, size_t i, Inds... inds) noexcept {
            return AtImpl(matrix[i], inds...);
        }

        template <typename T, typename... Inds>
        constexpr auto& at(T& matrix, Inds... inds) noexcept {
            return AtImpl(matrix, inds...);
        }

        template <typename F, typename... Vs>
        inline constexpr auto kFmatrix = GenFmatrix<F &&, Vs &&...>();

        template <typename F, typename... Vs>
        inline constexpr auto kFmatrixIndex = GenFmatrixIndex<F &&, Vs &&...>();

        template <typename F, typename... Vs>
        constexpr decltype(auto) VisitIndex(F&& vis, Vs&&... variants) {
            return at(kFmatrixIndex<F, Vs...>, variants.Index()...)(std::forward<F>(vis));
        }

    } // End of namespace cpp::variant::details

    template <typename F, typename... Vs>
    constexpr decltype(auto) Visit(F&& vis, Vs&&... variants) {
        if ((variants.ValuelessByException() || ...)) {
            throw BadVariantAccess();
        }
        return at(details::kFmatrix<F, Vs...>, variants.Index()...)(std::forward<F>(vis), std::forward<Vs>(variants)...);
    }

} // End of namespace cpp::variant

#endif //CPP_IMPLEMENTATIONS_VARIANT_UTILS_H
