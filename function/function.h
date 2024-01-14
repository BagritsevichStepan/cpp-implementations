#ifndef CPP_IMPLEMENTATIONS_FUNCTION_H
#define CPP_IMPLEMENTATIONS_FUNCTION_H

#include <type_traits>
#include <stdexcept>
#include <utility>

namespace cpp::function {

    class BadFunctionCall : std::runtime_error {
    public:
        explicit BadFunctionCall(const char* message) : runtime_error(message) {}
    };

    namespace details {

        using Storage = std::aligned_storage<sizeof(void*), alignof(void*)>;

        template <typename F>
        constexpr bool kFitsSmallStorage = sizeof(F) <= sizeof(F*) && alignof(F*) % alignof(F) == 0
                && std::is_nothrow_move_constructible_v<F>;

        template <typename T>
        constexpr const T* GetFunction(const Storage* storage);

        template <typename T>
        constexpr T* GetFunction(Storage* storage);

        template <typename F, typename... Args>
        class FunctionTypeDescriptor {
        private:
            constexpr FunctionTypeDescriptor(
                    F (*invoke)(Storage*, Args...),
                    void (*copy)(Storage&, const Storage*),
                    void (*move)(Storage&, Storage*),
                    void (*destroy)(Storage*)
            );

        public:
            F (*invoke_)(Storage*, Args...);
            void (*copy_)(Storage&, const Storage*);
            void (*move_)(Storage&, Storage*);
            void (*destroy_)(Storage*);

            template <typename T>
            static const FunctionTypeDescriptor* GetFunctionTypeDescriptor();

            static const FunctionTypeDescriptor* GetEmptyFunctionTypeDescriptor();
        };


    } // End of namespace cpp::function::details


    template <typename F, typename... Args>
    class Function;

    template <typename F, typename... Args>
    class Function<F(Args...)> {
    public:
        Function();

        template <typename T>
        explicit Function(T function);

        Function(const Function& other);
        Function(Function&& other) noexcept;
        Function& operator=(const Function& other);
        Function& operator=(Function&& other) noexcept;

        void Swap(Function& other);

        ~Function();

        F operator()(Args... args);

        template <typename T>
        T* target() noexcept;

        explicit operator bool() const noexcept;

    private:
        details::Storage storage_;
        const details::FunctionTypeDescriptor<F, Args...>* type_descriptor_;
    };

    template <typename F, typename... Args>
    void swap(Function<F(Args...)>& a, Function<F(Args...)>& b);


    // Implementation

    namespace details {

        template <typename T>
        constexpr const T* GetFunction(const Storage* storage) {
            if constexpr (kFitsSmallStorage<T>) {
                return reinterpret_cast<const T*>(storage);
            } else {
                return *reinterpret_cast<const T**>(storage);
            }
        }

        template <typename T>
        constexpr T* GetFunction(Storage* storage) {
            if constexpr (kFitsSmallStorage<T>) {
                return reinterpret_cast<T*>(storage);
            } else {
                return *reinterpret_cast<T**>(storage);
            }
        }

        template <typename F, typename... Args>
        constexpr FunctionTypeDescriptor<F, Args...>::FunctionTypeDescriptor(
                F (*invoke)(Storage*, Args...),
                void (*copy)(Storage&, const Storage*),
                void (*move)(Storage&, Storage*),
                void (*destroy)(Storage*)
        ) : invoke_(invoke), copy_(copy), move_(move), destroy_(destroy) {}

        template <typename F, typename... Args>
        template <typename T>
        const FunctionTypeDescriptor<F, Args...>* FunctionTypeDescriptor<F, Args...>::GetFunctionTypeDescriptor() {
            static constexpr FunctionTypeDescriptor<F, Args...> type_descriptor = {
                    [](Storage* storage, Args... args) -> F { // invoke
                        T* function = GetFunction<T>(storage);
                        return function->operator()(std::forward<Args>(args)...);
                    },
                    [](Storage& dst, const Storage* src) { // copy
                        const T* src_function = GetFunction<T>(src);
                        if constexpr (kFitsSmallStorage<T>) {
                            new (&dst) T(*src_function);
                        } else {
                            new (&dst) (T*)(new T(*src_function));
                        }
                    },
                    [](Storage& dst, Storage* src) { //move
                        T* src_function = GetFunction<T>(src);
                        if constexpr (kFitsSmallStorage<T>) {
                            auto p = std::move(*src_function);
                            new (&dst) T(p);
                        } else {
                            new (&dst) (T*)(std::move(src_function));
                        }
                    },
                    [](Storage* storage) { // destroy
                        T* function = GetFunction<T>(storage);
                        if (kFitsSmallStorage<T>) {
                            function->~T();
                        } else {
                            delete function;
                        }
                    }
            };

            return &type_descriptor;
        }

        template <typename F, typename... Args>
        const FunctionTypeDescriptor<F, Args...>* FunctionTypeDescriptor<F, Args...>::GetEmptyFunctionTypeDescriptor() {
            static constexpr FunctionTypeDescriptor<F, Args...> empty_type_descriptor = {
                    [](Storage* storage, Args... args) -> F { // invoke
                        throw BadFunctionCall("Empty function call");
                    },
                    [](Storage& dst, const Storage* src) {}, // copy
                    [](Storage& dst, Storage* src) {}, //move
                    [](Storage* storage) {} // destroy
            };

            return &empty_type_descriptor;
        }

    } // End of namespace cpp::function::details


    template<typename F, typename... Args>
    Function<F(Args...)>::Function() :
            type_descriptor_(details::FunctionTypeDescriptor<F, Args...>::GetEmptyFunctionTypeDescriptor()) {}

    template <typename F, typename... Args>
    template <typename T>
    Function<F(Args...)>::Function(T function) {
        type_descriptor_ = details::FunctionTypeDescriptor<F, Args...>::template GetFunctionTypeDescriptor<T>();
        if constexpr (details::kFitsSmallStorage<T>) {
            new (&storage_) T(std::move(function));
        } else {
            new (&storage_) (T*)(function);
        }
    }

    template<typename F, typename... Args>
    Function<F(Args...)>::Function(const Function& other) : type_descriptor_(other.type_descriptor_) {
        other.type_descriptor_->copy_(storage_, &other.storage_);
    }

    template<typename F, typename... Args>
    Function<F(Args...)>::Function(Function&& other) noexcept : type_descriptor_(other.type_descriptor_) {
        other.type_descriptor_->move_(storage_, &other.storage_);
        other.type_descriptor_ = details::FunctionTypeDescriptor<F, Args...>::GetEmptyFunctionTypeDescriptor();
    }

    template<typename F, typename... Args>
    Function<F(Args...)>& Function<F(Args...)>::operator=(const Function& other) {
        if (this != other) {
            Function tmp(other);
            Swap(tmp);
        }
        return *this;
    }

    template<typename F, typename... Args>
    Function<F(Args...)>& Function<F(Args...)>::operator=(Function&& other) noexcept {
        if (this != &other) {
            Function tmp(std::move(other));
            Swap(tmp);
        }
        return *this;
    }

    template<typename F, typename... Args>
    void Function<F(Args...)>::Swap(Function &other) { //TODO()
        using std::swap;
        swap(storage_, other.storage_);
        swap(type_descriptor_, other.type_descriptor_);
    }

    template <typename F, typename... Args>
    Function<F(Args...)>::~Function() {
        type_descriptor_->destroy_(&storage_);
    }

    template <typename F, typename... Args>
    F Function<F(Args...)>::operator()(Args... args) {
        return type_descriptor_->invoke_(&storage_, std::forward<Args>(args)...);
    }

    template <typename F, typename... Args>
    template <typename T>
    T* Function<F(Args...)>::target() noexcept {
        return type_descriptor_ == details::FunctionTypeDescriptor<F, Args...>::template GetFunctionTypeDescriptor<T>()
               ? details::GetFunction<T>(&storage_)
               : nullptr;
    }

    template <typename F, typename... Args>
    Function<F(Args...)>::operator bool() const noexcept {
        return type_descriptor_ != details::FunctionTypeDescriptor<F, Args...>::GetEmptyFunctionTypeDescriptor();
    }


    template <typename F, typename... Args>
    void swap(Function<F(Args...)>& a, Function<F(Args...)>& b) {
        a.Swap(b);
    }

} // End of namespace cpp::function

#endif //CPP_IMPLEMENTATIONS_FUNCTION_H
