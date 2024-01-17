#ifndef CPP_IMPLEMENTATIONS_VARIANT_STORAGE_H
#define CPP_IMPLEMENTATIONS_VARIANT_STORAGE_H

#include <cstdint>
#include <utility>
#include <type_traits>
#include "variant_constraints.h"
#include "variant_utils.h"

namespace cpp::variant {

    template <typename... Ts>
    class Variant;


    namespace details {

        class UninitializedStorageTag {};

        inline constexpr UninitializedStorageTag kUninitializedStorageTag;

        template <bool TriviallyDestructible, typename... Ts>
        union MegaUnion {
            constexpr MegaUnion() = default;
            constexpr MegaUnion(UninitializedStorageTag) noexcept {}
        };

        template <bool TriviallyDestructible, typename T, typename... Rest>
        union MegaUnion<TriviallyDestructible, T, Rest...> {
            constexpr MegaUnion() : val() {}

            constexpr MegaUnion(UninitializedStorageTag tag) : rest(tag) {}

            template <typename... Args>
            constexpr MegaUnion(InPlaceIndex<0>, Args&&... args) : val(std::forward<Args>(args)...) {}

            template <size_t N, typename... Args>
            constexpr MegaUnion(InPlaceIndex<N>, Args&&... args) : rest(kInPlaceIndex<N - 1>, std::forward<Args>(args)...) {};

            template <size_t N, typename... Args>
            constexpr decltype(auto) ConstructInternalValue(InPlaceIndex<N>, Args&&... args) {
                return rest.ConstructInternalValue(kInPlaceIndex<N - 1>, std::forward<Args>(args)...);
            }

            template <typename... Args>
            constexpr T& ConstructInternalValue(InPlaceIndex<0>, Args&&... args) {
                new (const_cast<std::remove_cv_t<T>*>(std::addressof(val))) T(std::forward<Args>(args)...);
                return val;
            }

            template <size_t N>
            constexpr auto& Get(InPlaceIndex<N>) noexcept {
                return rest.Get(kInPlaceIndex<N - 1>);
            }

            constexpr auto& Get(InPlaceIndex<0>) noexcept {
                return val;
            }

            template <size_t N>
            constexpr const auto& Get(InPlaceIndex<N>) const noexcept {
                return rest.Get(kInPlaceIndex<N - 1>);
            }

            constexpr const auto& Get(InPlaceIndex<0>) const noexcept {
                return val;
            }

            template <size_t N>
            constexpr void DestroyInternalValue(InPlaceIndex<N>) {}

            ~MegaUnion() = default;

            T val;
            MegaUnion<TriviallyDestructible, Rest...> rest;
        };

        template <typename T, typename... Rest>
        union MegaUnion<false, T, Rest...> {
            constexpr MegaUnion() : val() {}

            constexpr MegaUnion(UninitializedStorageTag tag) : rest(tag) {}

            template <typename... Args>
            constexpr MegaUnion(InPlaceIndex<0>, Args&&... args) : val(std::forward<Args>(args)...) {}

            template <size_t N, typename... Args>
            constexpr MegaUnion(InPlaceIndex<N>, Args&&... args) : rest(kInPlaceIndex<N - 1>, std::forward<Args>(args)...) {}

            template <size_t N, typename... Args>
            constexpr decltype(auto) ConstructInternalValue(InPlaceIndex<N>, Args&&... args) {
                return rest.ConstructInternalValue(kInPlaceIndex<N - 1>, std::forward<Args>(args)...);
            }

            template <typename... Args>
            constexpr T& ConstructInternalValue(InPlaceIndex<0>, Args&&... args) {
                new (const_cast<std::remove_cv_t<T>*>(std::addressof(val))) T(std::forward<Args>(args)...);
                return val;
            }

            template <size_t N>
            constexpr auto& Get(InPlaceIndex<N>) noexcept {
                return rest.Get(kInPlaceIndex<N - 1>);
            }

            constexpr auto& Get(InPlaceIndex<0>) noexcept {
                return val;
            }

            template <size_t N>
            constexpr const auto& Get(InPlaceIndex<N>) const noexcept {
                return rest.Get(kInPlaceIndex<N - 1>);
            }

            constexpr const auto& Get(InPlaceIndex<0>) const noexcept {
                return val;
            }

            template <size_t N>
            constexpr void DestroyInternalValue(InPlaceIndex<N>) {
                rest.DestroyInternalValue(kInPlaceIndex<N - 1>);
            }

            constexpr void DestroyInternalValue(InPlaceIndex<0>) {
                val.~T();
            }

            ~MegaUnion() {}

        protected:
            T val;
            MegaUnion<false, Rest...> rest;
        };

        template <bool TriviallyDestructible, typename... Ts>
        struct VariantStorage {
            constexpr VariantStorage() : val(), ind(0) {}

            constexpr VariantStorage(UninitializedStorageTag tag) : val(tag) {}

            template <size_t N, typename... Args>
            constexpr VariantStorage(InPlaceIndex<N> ind, Args&&... args)
                    : val(ind, std::forward<Args>(args)...), ind(N) {}

            template <size_t N, typename... Args>
            decltype(auto) ConstructInternalValue(InPlaceIndex<N> ind_, Args&&... args) {
                auto& res = val.ConstructInternalValue(ind_, std::forward<Args>(args)...);
                ind = N;
                return res;
            }

            constexpr void DestroyInternalValue() {
                if (ind != kVariantNPos) {
                    details::VisitIndex([this](auto ind_) { this->val.DestroyInternalValue(kInPlaceIndex<ind_()>); },
                            *static_cast<Variant<Ts...>*>(this));
                }
            }

            ~VariantStorage() = default;

        protected:
            MegaUnion<(std::is_trivially_destructible_v<Ts> && ...), Ts...> val;
            size_t ind{kVariantNPos};
        };

        template <typename... Ts>
        struct VariantStorage<false, Ts...> {
            constexpr VariantStorage() : val(), ind(0) {}

            constexpr VariantStorage(UninitializedStorageTag tag) : val(tag) {}

            template <size_t N, typename... Args>
            constexpr VariantStorage(InPlaceIndex<N> ind, Args&&... args)
                    : val(ind, std::forward<Args>(args)...), ind(N) {}

            template <size_t N, typename... Args>
            decltype(auto) ConstructInternalValue(InPlaceIndex<N> ind_, Args&&... args) {
                auto& res = val.ConstructInternalValue(ind_, std::forward<Args>(args)...);
                ind = N;
                return res;
            }

            constexpr void DestroyInternalValue() {
                if (ind != kVariantNPos) {
                    details::VisitIndex([this](auto ind_) { this->val.DestroyInternalValue(kInPlaceIndex<ind_()>); },
                                         *static_cast<Variant<Ts...>*>(this));
                }
            }

            constexpr ~VariantStorage() {
                DestroyInternalValue();
            }

        protected:
            MegaUnion<(std::is_trivially_destructible_v<Ts> && ...), Ts...> val;
            size_t ind{kVariantNPos};
        };

        template <typename... Ts>
        using VariantStorageType = VariantStorage<(std::is_trivially_destructible_v<Ts> && ...), Ts...>;

    } // End of namespace cpp::variant::details

} // End of namespace cpp::variant

#endif //CPP_IMPLEMENTATIONS_VARIANT_STORAGE_H
