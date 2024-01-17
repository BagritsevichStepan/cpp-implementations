#ifndef CPP_IMPLEMENTATIONS_VARIANT_H
#define CPP_IMPLEMENTATIONS_VARIANT_H

#include "variant_storage.h"

namespace cpp::variant {

    template <typename... Ts>
    class Variant : private details::VariantStorageType<Ts...> {
    public:
        using Base = details::VariantStorageType<Ts...>;
        using ZeroAlt = details::At<0, Ts...>;

        constexpr Variant() noexcept(std::is_nothrow_default_constructible_v<ZeroAlt>) = delete;

        constexpr Variant() noexcept(std::is_nothrow_default_constructible_v<ZeroAlt>)
        requires std::is_default_constructible_v<ZeroAlt> = default;

        template <typename T, typename ChosenTypeInfo = details::ChosenOverload<T, Ts...>, typename ChosenT = typename ChosenTypeInfo::Type, size_t Index = ChosenTypeInfo::index>
        constexpr Variant(T&& t) noexcept(std::is_nothrow_constructible_v<T, ChosenT>)
        requires details::IsConvertibleToChosenTypeVariant<Variant, T, ChosenT, Ts...>
                : Base(kInPlaceIndex<Index>, std::forward<T>(t)) {}

        template <typename T, typename ChosenTypeInfo = details::ChosenOverload<T, Ts...>, typename ChosenT = typename ChosenTypeInfo::Type, size_t Index = ChosenTypeInfo::index>
        constexpr Variant& operator=(T&& t) noexcept(std::is_nothrow_assignable_v<ChosenT&, T> && std::is_nothrow_constructible_v<ChosenT, T>)
        requires details::IsConvertibleToChosenTypeVariant<Variant, T, ChosenT, Ts...> {
            if (this->Index() == Index) {
                Get<Index>(*this) = std::forward<T>(t);
            } else if (std::is_nothrow_constructible_v<ChosenT, T> || !std::is_nothrow_move_constructible_v<ChosenT>) {
                Emplace<Index>(std::forward<T>(t));
            } else {
                Emplace<Index>(ChosenT(std::forward<T>(t)));
            }
            return *this;
        }

        constexpr Variant(const Variant& other) = delete;

        constexpr Variant(const Variant& other) requires(details::IsCopyConstructible<Ts...>) : Base(details::kUninitializedStorageTag) {
            if (!other.ValuelessByException()) {
                details::VisitIndex([this, &other](auto ind_) {
                            this->ConstructInternalValue(kInPlaceIndex<ind_()>, Get<ind_()>(other));
                        },
                        other);
            } else {
                this->MakeValueless();
            }
        }

        constexpr Variant(const Variant&) requires(details::IsTriviallyCopyConstructible<Ts...>) = default;

        constexpr Variant& operator=(const Variant& other) = delete;

        constexpr Variant& operator=(const Variant& other) requires(details::IsCopyAssignable<Ts...>) {
            if (other.ValuelessByException()) {
                if (this->ValuelessByException()) {
                    return *this;
                } else {
                    this->DestroyInternalValue();
                    this->MakeValueless();
                }
            }
            details::VisitIndex(
                    [this, &other](auto ind_) {
                        if (this->Index() == ind_()) {
                            Get<ind_()>(*this) = Get<ind_()>(other);
                        } else {
                            using OtherAlt = details::At<ind_(), Ts...>;
                            if (std::is_nothrow_copy_constructible_v<OtherAlt> || !std::is_nothrow_move_constructible_v<OtherAlt>) {
                                Emplace<ind_()>(Get<ind_()>(other));
                            } else {
                                this->operator=(Variant(other));
                            }
                        }
                    },
                    other);
            return *this;
        }

        constexpr Variant& operator=(const Variant& other) requires(details::IsTriviallyCopyAssignable<Ts...>) = default;

        constexpr Variant(Variant&& other) = delete;

        constexpr Variant(Variant &&other) noexcept((std::is_nothrow_move_constructible_v<Ts> && ...)) requires(details::IsMoveConstructible<Ts...>)
                : Base(details::kUninitializedStorageTag) {
            if (!other.ValuelessByException()) {
                details::VisitIndex(
                        [this, &other](auto ind_) {
                            this->ConstructInternalValue(kInPlaceIndex<ind_()>, Get<ind_()>(std::move(other)));
                        },
                        other);
            } else {
                this->MakeValueless();
            }
        }

        constexpr Variant(Variant&& other) noexcept((std::is_nothrow_move_constructible_v<Ts> && ...))
        requires(details::IsTriviallyMoveConstructible<Ts...>) = default;

        constexpr Variant& operator=(Variant&& other) = delete;

        constexpr Variant& operator=(Variant&& other) noexcept(((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_move_assignable_v<Ts>) && ...))
        requires(details::IsMoveAssignable<Ts...>) {
            if (other.ValuelessByException()) {
                if (this->ValuelessByException()) {
                    return *this;
                } else {
                    this->DestroyInternalValue();
                    this->MakeValueless();
                    return *this;
                }
            }
            details::VisitIndex(
                    [this, &other](auto ind_) {
                        if (this->Index() == ind_()) {
                            Get<ind_()>(*this) = Get<ind_()>(std::move(other));
                        } else {
                            Emplace<ind_()>(Get<ind_()>(std::move(other)));
                        }
                    },
                    other);
            return *this;
        }

        constexpr Variant& operator=(Variant&& other) noexcept(((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_move_assignable_v<Ts>) && ...))
        requires(details::IsTriviallyMoveAssignable<Ts...>) = default;

        template <typename T, size_t Index = details::kFindInPackValue<T, Ts...>, typename... Args>
        constexpr explicit Variant(InPlaceType<T>, Args&&... args)
        requires(std::is_constructible_v<T, Args...> && details::kOccursOnlyOnceValue<T, Ts...>)
            : Base(kInPlaceIndex<Index>, std::forward<Args>(args)...) {}

        template <size_t N, typename T_N = details::At<N, Ts...>, typename... Args>
        constexpr explicit Variant(InPlaceIndex<N> ind, Args&&... args)
        requires(!std::is_same_v<T_N, void> && std::is_constructible_v<T_N, Args...>)
            : Base(ind, std::forward<Args>(args)...) {}

        template <std::size_t N, typename T_N = details::At<N, Ts...>, typename... Args>
        constexpr T_N& Emplace(Args&&... args) requires(std::is_constructible_v<T_N, Args...>) {
            static_assert(N < sizeof...(Ts));

            Base::DestroyInternalValue();
            this->MakeValueless();
            return Base::ConstructInternalValue(kInPlaceIndex<N>, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args, size_t Index = details::kFindInPackValue<T, Ts...>>
        constexpr T& Emplace(Args&&... args) requires(std::is_constructible_v<T, Args...> && details::kOccursOnlyOnceValue<T, Ts...>) {
            return Emplace<Index, T>(std::forward<Args>(args)...);
        }

        constexpr void Swap(Variant& other) noexcept(noexcept(((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_swappable_v<Ts>) && ...))) {
            if (this->ValuelessByException() && other.ValuelessByException()) {
                return;
            }
            if (other.Index() == Index()) {
                details::VisitIndex(
                        [this, &other](auto ind_) {
                            using std::swap;
                            swap(Get<ind_()>(*this), Get<ind_()>(other));
                            swap(this->ind, other.ind);
                        },
                        other);
            } else {
                using std::swap;
                swap(*this, other);
            }
        }

        [[nodiscard]] constexpr bool ValuelessByException() const noexcept {
            return Index() == kVariantNPos;
        }

        [[nodiscard]] constexpr size_t Index() const noexcept {
            return this->ind;
        }

        constexpr void MakeValueless() noexcept {
            this->ind = kVariantNPos;
        }

        constexpr ~Variant() = default;

        template <size_t N, typename... Ts_>
        friend constexpr VariantAlternativeType<N, Variant<Ts_...>>& Get(Variant<Ts_...>& v);

        template <size_t N, typename... Ts_>
        friend constexpr const VariantAlternativeType<N, Variant<Ts_...>>& Get(const Variant<Ts_...>& v);

        template <typename T, typename... Ts_, size_t Index>
        friend constexpr T& Get(Variant<Ts_...>& v);

        template <typename T, typename... Ts_, size_t Index>
        friend constexpr const T& Get(const Variant<Ts_...>& v);

        template <bool TriviallyDestructible, typename... Ts_>
        friend struct details::VariantStorage;

    };

    template <typename... Ts>
    constexpr bool operator==(const Variant<Ts...>& v, const Variant<Ts...>& w) noexcept {
        if (v.Index() != w.Index()) {
            return false;
        }
        if (v.ValuelessByException()) {
            return true;
        }
        return details::VisitIndex([&v, &w](auto ind_) -> bool { return Get<ind_()>(v) == Get<ind_()>(w); }, v);
    }

    template <typename... Ts>
    constexpr bool operator!=(const Variant<Ts...>& v, const Variant<Ts...>& w) noexcept {
        return !(v == w);
    }

    template <typename... Ts>
    constexpr bool operator<(const Variant<Ts...>& v, const Variant<Ts...>& w) noexcept {
        if (w.ValuelessByException()) {
            return false;
        }
        if (v.ValuelessByException()) {
            return true;
        }
        return details::VisitIndex(
                [&v, &w](auto ind1, auto ind2) -> bool {
                    if (ind1() < ind2()) {
                        return true;
                    }
                    if (ind1() > ind2()) {
                        return false;
                    }
                    return Get<ind1()>(v) < Get<ind1()>(w);
                },
                v, w);
    }

    template <typename... Ts>
    constexpr bool operator>(const Variant<Ts...>& v, const Variant<Ts...>& w) noexcept {
        return w < v;
    }

    template <typename... Ts>
    constexpr bool operator<=(const Variant<Ts...>& v, const Variant<Ts...>& w) noexcept {
        return !(v > w);
    }

    template <typename... Ts>
    constexpr bool operator>=(const Variant<Ts...>& v, const Variant<Ts...>& w) noexcept {
        return !(v < w);
    }

} // End of namespace cpp::variant

#endif //CPP_IMPLEMENTATIONS_VARIANT_H
