#ifndef CPP_IMPLEMENTATIONS_SHARED_PTR_H
#define CPP_IMPLEMENTATIONS_SHARED_PTR_H

#include <iostream>
#include <memory>
#include <cassert>
#include <type_traits>

namespace cpp::pointer {

    template <typename T>
    class SharedPointer;

    template <typename T>
    class WeakPointer;

    template <typename T, typename... Args>
    SharedPointer<T> MakeShared(Args&&... args);

    namespace details {

        template <typename T>
        class ControlBlock {
        public:
            ControlBlock() = default;

            ControlBlock(const ControlBlock&) = delete;
            ControlBlock(ControlBlock&&) = delete;
            ControlBlock& operator=(const ControlBlock&) = delete;
            ControlBlock& operator=(ControlBlock&&) = delete;

            void AddStrongPointer();
            void RemoveStrongPointer();
            void AddWeakPointer();
            void RemoveWeakPointer();

            virtual T* GetPointer() = 0;

            [[nodiscard]] bool ControlBlockMustBeDeleted() const;

            virtual ~ControlBlock() = default;

            template <typename _T, typename... Args>
            friend SharedPointer<_T> cpp::pointer::MakeShared(Args&&... args);

            friend class WeakPointer<T>;
            friend class SharedPointer<T>;

        private:
            [[nodiscard]] bool DataMustBeDeleted() const;
            void DeleteData();

            void CheckInvariant() const;

        protected:
            virtual void DestructData() = 0;

            bool data_is_deleted_{false};

        private:
            size_t strong_ptr_count_{0};
            size_t ptr_count_{0};

        };

        template <typename T, typename Deleter>
        class PointerControlBlock final : public ControlBlock<T>, public Deleter {
        public:
            PointerControlBlock(T* pointer, Deleter deleter) noexcept;

            PointerControlBlock(const PointerControlBlock&) = delete;
            PointerControlBlock(PointerControlBlock&&) = delete;
            PointerControlBlock& operator=(const PointerControlBlock&) = delete;
            PointerControlBlock& operator=(PointerControlBlock&&) = delete;

            T* GetPointer() override;
            void DestructData() override;

            ~PointerControlBlock();

        private:
            using ControlBlock<T>::data_is_deleted_;
            T* data_;
        };

        template <typename T>
        class InplaceControlBlock final : public ControlBlock<T> {
        public:
            template <typename... Args>
            explicit InplaceControlBlock(Args&&... args);

            InplaceControlBlock(const InplaceControlBlock&) = delete;
            InplaceControlBlock(InplaceControlBlock&&) = delete;
            InplaceControlBlock& operator=(const InplaceControlBlock&) = delete;
            InplaceControlBlock& operator=(InplaceControlBlock&&) = delete;

            T* GetPointer() override;
            void DestructData() override;

            ~InplaceControlBlock();

        private:
            using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
            using ControlBlock<T>::data_is_deleted_;

            Storage data_;

        };

    }


    template <typename T>
    class SharedPointer final {
    private:
        SharedPointer(details::ControlBlock<T>* control_block, T* data);

    public:
        SharedPointer() = default;

        template <typename _T, typename Deleter = std::default_delete<_T>,
                typename = std::enable_if_t<std::is_convertible_v<_T, T>, bool>>
        explicit SharedPointer(_T* data, Deleter&& deleter = Deleter());

        SharedPointer(const SharedPointer& other);
        SharedPointer(SharedPointer&& other) noexcept;
        SharedPointer& operator=(const SharedPointer& other);
        SharedPointer& operator=(SharedPointer&& other) noexcept;

        template <typename _T, typename = std::enable_if_t<std::is_convertible_v<_T, T>, bool>>
        explicit SharedPointer(const SharedPointer<_T>& other);

        void Swap(SharedPointer<T>& other) noexcept;

        T* Get() const noexcept;

        T& operator*() const noexcept;
        T* operator->() const noexcept;

        [[nodiscard]] size_t UseCount() const noexcept;

        void Reset();

        template <typename _T, typename Deleter = std::default_delete<_T>,
                typename = std::enable_if_t<std::is_convertible_v<_T, T>, bool>>
        void Reset(_T* data, Deleter&& deleter = Deleter());

        ~SharedPointer();

        friend class WeakPointer<T>;

        template <typename _T, typename... Args>
        friend SharedPointer<_T> MakeShared(Args&&... args);

    private:
        details::ControlBlock<T>* control_block_{nullptr};
        T* pointer_{nullptr};
    };

    template <typename T>
    class WeakPointer final {
    public:
        WeakPointer() noexcept = default;
        explicit WeakPointer(const SharedPointer<T>& shared_pointer);

        WeakPointer(const WeakPointer& other);
        WeakPointer(WeakPointer&& other) noexcept;
        WeakPointer& operator=(const WeakPointer& other);
        WeakPointer& operator=(WeakPointer&& other) noexcept;

        void Swap(WeakPointer& other);

        SharedPointer<T> Lock();

        [[nodiscard]] size_t UseCount() const noexcept;
        [[nodiscard]] bool IsExpired() const noexcept;

        ~WeakPointer();

    private:
        details::ControlBlock<T>* control_block_{nullptr};
    };


    // Implementation
    template <typename T>
    SharedPointer<T>::SharedPointer(details::ControlBlock<T>* control_block, T* data) : pointer_(data), control_block_(control_block) {
        if (control_block_) {
            control_block_->AddStrongPointer();
        }
    }

    template <typename T>
    template <typename _T, typename Deleter, typename>
    SharedPointer<T>::SharedPointer(_T* data, Deleter&& deleter) : pointer_(data) {
        try {
            control_block_ = new details::PointerControlBlock<_T, Deleter>(data, std::forward<Deleter&&>(deleter));
        } catch (...) {
            delete data;
            throw;
        }

        if (control_block_) {
            control_block_->AddStrongPointer();
        }
    }

    template <typename T>
    SharedPointer<T>::SharedPointer(const SharedPointer& other)
            : control_block_(other.control_block_), pointer_(other.pointer_) {
        if (control_block_) {
            control_block_->AddStrongPointer();
        }
    }

    template <typename T>
    SharedPointer<T>::SharedPointer(SharedPointer&& other) noexcept {
        control_block_ = other.control_block_;
        pointer_ = other.pointer_;

        other.control_block_ = nullptr;
        other.pointer_ = nullptr;
    }

    template <typename T>
    SharedPointer<T>& SharedPointer<T>::operator=(const SharedPointer& other) {
        if (this != &other) {
            SharedPointer(other).Swap(*this);
        }
        return *this;
    }

    template<typename T>
    SharedPointer<T>& SharedPointer<T>::operator=(SharedPointer&& other) noexcept {
        if (this != &other) {
            SharedPointer(std::forward<SharedPointer&&>(other)).Swap(*this);
        }
        return *this;
    }

    template<typename T>
    void SharedPointer<T>::Swap(SharedPointer<T> &other) noexcept {
        using std::swap;
        swap(control_block_, other.control_block_);
        swap(pointer_, other.pointer_);
    }

    template<typename T>
    T* SharedPointer<T>::Get() const noexcept {
        return pointer_;
    }

    template <typename T>
    T& SharedPointer<T>::operator*() const noexcept {
        return *pointer_;
    }

    template<typename T>
    T* SharedPointer<T>::operator->() const noexcept {
        return pointer_;
    }

    template<typename T>
    size_t SharedPointer<T>::UseCount() const noexcept {
        return control_block_ ? control_block_->strong_ptr_count_ : 0;
    }

    template<typename T>
    void SharedPointer<T>::Reset() {
        SharedPointer().Swap(*this);
    }

    template<typename T>
    template<typename _T, typename Deleter, typename>
    void SharedPointer<T>::Reset(_T *data, Deleter &&deleter) {
        SharedPointer(data, deleter).Swap(*this);
    }

    template<typename T>
    SharedPointer<T>::~SharedPointer() {
        if (control_block_) {
            control_block_->RemoveStrongPointer();
            if (control_block_->ControlBlockMustBeDeleted()) {
                delete control_block_;
            }
        }
    }

    // WeakPointer
    template <typename T>
    WeakPointer<T>::WeakPointer(const SharedPointer<T>& shared_pointer)
            : control_block_(shared_pointer.control_block_) {
        if (control_block_) {
            control_block_->AddWeakPointer();
        }
    }

    template<typename T>
    WeakPointer<T>::WeakPointer(const WeakPointer& other) : control_block_(other.control_block_) {
        if (control_block_) {
            control_block_->AddWeakPointer();
        }
    }

    template<typename T>
    WeakPointer<T>::WeakPointer(WeakPointer&& other) noexcept {
        control_block_ = other.control_block_;
        other.control_block_ = nullptr;
    }

    template<typename T>
    WeakPointer<T>& WeakPointer<T>::operator=(const WeakPointer& other) {
        if (this != &other) {
            WeakPointer(other).Swap(*this);
        }
        return *this;
    }

    template <typename T>
    WeakPointer<T>& WeakPointer<T>::operator=(WeakPointer&& other) noexcept {
        if (this != &other) {
            WeakPointer(std::forward<WeakPointer&&>(other)).Swap(*this);
        }
        return *this;
    }

    template <typename T>
    void WeakPointer<T>::Swap(WeakPointer& other) {
        using std::swap;
        swap(control_block_, other.control_block_);
    }

    template<typename T>
    SharedPointer<T> WeakPointer<T>::Lock() {
        return IsExpired() ? SharedPointer<T>() : SharedPointer<T>(control_block_, control_block_->GetPointer());
    }

    template<typename T>
    size_t WeakPointer<T>::UseCount() const noexcept {
        return control_block_ ? control_block_->strong_ptr_count_ : 0;
    }

    template<typename T>
    bool WeakPointer<T>::IsExpired() const noexcept {
        return control_block_ && !control_block_->strong_ptr_count_;
    }

    template<typename T>
    WeakPointer<T>::~WeakPointer() {
        if (control_block_) {
            control_block_->RemoveWeakPointer();
            if (control_block_->ControlBlockMustBeDeleted()) {
                delete control_block_;
            }
        }
    }


    // ControlBlock
    namespace details {

        template <typename T>
        void ControlBlock<T>::AddStrongPointer() {
            CheckInvariant();
            strong_ptr_count_++;
            ptr_count_++;
        }

        template <typename T>
        void ControlBlock<T>::RemoveStrongPointer() {
            CheckInvariant();
            strong_ptr_count_--;
            ptr_count_--;
            if (DataMustBeDeleted()) {
                DeleteData();
            }
        }

        template <typename T>
        void ControlBlock<T>::AddWeakPointer() {
            ptr_count_++;
            CheckInvariant();
        }

        template <typename T>
        void ControlBlock<T>::RemoveWeakPointer() {
            ptr_count_--;
            CheckInvariant();
        }

        template <typename T>
        bool ControlBlock<T>::ControlBlockMustBeDeleted() const {
            return !ptr_count_;
        }

        template <typename T>
        bool ControlBlock<T>::DataMustBeDeleted() const {
            return !data_is_deleted_ && !strong_ptr_count_;
        }

        template <typename T>
        void ControlBlock<T>::DeleteData() {
            data_is_deleted_ = true;
            DestructData();
        }

        template <typename T>
        void ControlBlock<T>::CheckInvariant() const {
            assert(strong_ptr_count_ <= ptr_count_);
        }


        template <typename T, typename Deleter>
        PointerControlBlock<T, Deleter>::PointerControlBlock(T* pointer, Deleter deleter) noexcept
                : data_(pointer), Deleter(std::move(deleter)) {}

        template <typename T, typename Deleter>
        T* PointerControlBlock<T, Deleter>::GetPointer() {
            return data_;
        }

        template <typename T, typename Deleter>
        void PointerControlBlock<T, Deleter>::DestructData() {
            (*static_cast<Deleter*>(this))(data_);
        }

        template <typename T, typename Deleter>
        PointerControlBlock<T, Deleter>::~PointerControlBlock() {
            if (!data_is_deleted_) {
                DestructData();
            }
        }

        template <typename T>
        template <typename... Args>
        InplaceControlBlock<T>::InplaceControlBlock(Args&&... args) {
            new (&data_) T(std::forward<Args>(args)...);
        }

        template <typename T>
        T* InplaceControlBlock<T>::GetPointer() {
            return reinterpret_cast<T*>(&data_);
        }

        template <typename T>
        void InplaceControlBlock<T>::DestructData() {
            GetPointer()->~T();
        }

        template <typename T>
        InplaceControlBlock<T>::~InplaceControlBlock() {
            if (!data_is_deleted_) {
                DestructData();
            }
        }

    }


    template <typename T, typename... Args>
    SharedPointer<T> MakeShared(Args&&... args) {
        auto* control_block = new details::InplaceControlBlock<T>(std::forward<Args>(args)...);
        return SharedPointer<T>(control_block, control_block->GetPointer());
    }

} // End of namespace cpp::pointer

#endif //CPP_IMPLEMENTATIONS_SHARED_PTR_H
