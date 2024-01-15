#ifndef CPP_IMPLEMENTATIONS_INTRUSIVE_LIST_H
#define CPP_IMPLEMENTATIONS_INTRUSIVE_LIST_H

#include <type_traits>
#include <ostream>

namespace cpp::intrusive {

    class DefaultTag;

    template <typename Tag = DefaultTag>
    class ListElement;

    template <typename T, typename Tag>
    concept IsListElement = std::is_base_of_v<ListElement<Tag>, T>;

    template <typename T, typename Tag = DefaultTag>
    requires IsListElement<T, Tag>
    class List;


    class ListElementBase {
    protected:
        ListElementBase();
        ~ListElementBase();

    private:
        void InsertBetween(ListElementBase* left, ListElementBase* right);
        void InsertBefore(ListElementBase& element);
        void InsertAfter(ListElementBase& element);
        void Unlink();

    private:
        ListElementBase* prev_{this};
        ListElementBase* next_{this};

        template <typename T, typename Tag>
        requires IsListElement<T, Tag>
        friend class List;

    };

    template <typename Tag>
    class ListElement: private ListElementBase {
    protected:
        ListElement() = default;
        ~ListElement() = default;

    public:
        ListElement(const ListElement&) = delete;
        ListElement(const ListElement&&) = delete;
        ListElement& operator=(const ListElement&) = delete;
        ListElement& operator=(const ListElement&&) = delete;

        template <typename T, typename Tag_>
        requires IsListElement<T, Tag_>
        friend class List;

    };


    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    class List {
    private:
        template <bool isConstType>
        class Iterator;

    public:
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        List() = default;

        List(const List&) = delete;
        List& operator=(const List&) = delete;

        List(List&& other) noexcept;
        List& operator=(List&& other) noexcept;

        ~List();

        void Swap(List& other) noexcept;

        T* Front() const noexcept;
        T* Back() const noexcept;

        iterator Insert(const_iterator position, T& element);

        iterator Erase(const_iterator position);

        void PushBack(T& element);
        void PushFront(T& element);
        void PopBack();
        void PopFront();

        iterator begin();
        iterator end();
        const_iterator cbegin();
        const_iterator cend();

        reverse_iterator rbegin();
        reverse_iterator rend();
        const_reverse_iterator crbegin();
        const_reverse_iterator crend();

        friend std::ostream &operator<<(std::ostream& os, const List& list) {
            static auto print_list_element = [&os, &list](ListElementBase* current_element) {
                os << *ToTemplateType(current_element);
                if (current_element->next_ != &list.empty_element_) {
                    os << ", ";
                }
            };

            os << "List=[";
            list.TraverseListAndInvoke(std::move(print_list_element));
            os << "]";
            return os;
        }

    private:
        template <typename F>
        void TraverseListAndInvoke(F&& function) const;

        static ListElementBase* ToListElementBase(T& element);
        static T* ToTemplateType(ListElementBase* list_element_base);

        template <bool isConstType>
        class Iterator {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = std::conditional_t<isConstType, const T*, T*>;
            using reference = std::conditional_t<isConstType, const T&, T&>;

            explicit Iterator(ListElementBase* element);

            template <bool _isConstType>
            requires isConstType
            Iterator(const Iterator<_isConstType>& other);

            Iterator& operator++();
            Iterator& operator--();
            Iterator operator++(int);
            Iterator operator--(int);

            bool operator==(const Iterator& other) const noexcept;
            bool operator!=(const Iterator& other) const noexcept;

            reference operator*() const;
            pointer operator->() const;

            friend class List;

        private:
            ListElementBase* current_element_;
        };

    private:
        ListElementBase empty_element_{};
    };

    template <typename T, typename Tag>
    void swap(List<T, Tag>& first, List<T, Tag>& second);


    // Implementation
    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::List(List&& other) noexcept {
        swap(other);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>& List<T, Tag>::operator=(List&& other) noexcept {
        swap(other);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::~List() {
        ListElementBase* current_element = &empty_element_;
        do {
            ListElementBase* const next_element = current_element->next_;
            current_element->Unlink();
            current_element = next_element;
        } while (current_element != &empty_element_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    void List<T, Tag>::Swap(List& other) noexcept {
        using std::swap;
        swap(empty_element_.next_->prev_, other.empty_element_.next_->prev_);
        swap(empty_element_.prev_->next_, other.empty_element_.prev_->next_);
        swap(empty_element_.next_, other.empty_element_.next_);
        swap(empty_element_.prev_, other.empty_element_.prev_);
    }


    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    T* List<T, Tag>::Front() const noexcept {
        return ToTemplateType(empty_element_.next_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    T* List<T, Tag>::Back() const noexcept {
        return ToTemplateType(empty_element_.prev_);
    }

    template<typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::iterator List<T, Tag>::Insert(List::const_iterator position, T& element) {
        auto position_as_base = position.current_element_;
        auto element_as_base = ToListElementBase(element);
        element_as_base->Unlink();
        element_as_base->InsertBefore(*position_as_base);
        return iterator(element_as_base);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    void List<T, Tag>::PushBack(T &element) {
        auto element_as_base = ToListElementBase(element);
        element_as_base->InsertBefore(empty_element_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    void List<T, Tag>::PushFront(T &element) {
        auto element_as_base = ToListElementBase(element);
        element_as_base->InsertAfter(empty_element_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    void List<T, Tag>::PopBack() {
        empty_element_.prev_->Unlink();
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    void List<T, Tag>::PopFront() {
        empty_element_.next_->Unlink();
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::iterator List<T, Tag>::Erase(List::const_iterator position) {
        iterator result = iterator(position.current_element_->next_);
        position.current_element_->Unlink();
        return result;
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <typename F>
    void List<T, Tag>::TraverseListAndInvoke(F&& function) const {
        for (auto cur = empty_element_.next_; cur != &empty_element_; cur = cur->next_) {
            function(cur);
        }
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    ListElementBase* List<T, Tag>::ToListElementBase(T& element) {
        return static_cast<ListElementBase*>(&element);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    T* List<T, Tag>::ToTemplateType(ListElementBase* list_element_base) {
        return static_cast<T*>(list_element_base);
    }


    // Iterators
    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::iterator List<T, Tag>::begin() {
        return iterator(empty_element_.next_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::iterator List<T, Tag>::end() {
        return iterator(&empty_element_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::const_iterator List<T, Tag>::cbegin() {
        return const_iterator(empty_element_.next_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::const_iterator List<T, Tag>::cend() {
        return const_iterator(&empty_element_);
    }

    // Reverse iterators
    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::reverse_iterator List<T, Tag>::rbegin() {
        return reverse_iterator(empty_element_.prev_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::reverse_iterator List<T, Tag>::rend() {
        return reverse_iterator(&empty_element_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::const_reverse_iterator List<T, Tag>::crbegin() {
        return const_reverse_iterator(empty_element_.prev_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    List<T, Tag>::const_reverse_iterator List<T, Tag>::crend() {
        return const_reverse_iterator(&empty_element_);
    }


    // Iterator
    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    List<T, Tag>::Iterator<isConstType>::Iterator(ListElementBase* element) : current_element_(element) {}

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    template <bool _isConstType>
    requires isConstType
    List<T, Tag>::Iterator<isConstType>::Iterator(const Iterator<_isConstType>& other)
        : current_element_(other.current_element_) {}

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    List<T, Tag>::Iterator<isConstType>& List<T, Tag>::Iterator<isConstType>::operator++() {
        current_element_ = current_element_->next_;
        return *this;
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    List<T, Tag>::Iterator<isConstType>& List<T, Tag>::Iterator<isConstType>::operator--() {
        current_element_ = current_element_->prev_;
        return *this;
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    List<T, Tag>::Iterator<isConstType> List<T, Tag>::Iterator<isConstType>::operator++(int) {
        current_element_ = current_element_->next_;
        return Iterator(current_element_->prev_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    List<T, Tag>::Iterator<isConstType> List<T, Tag>::Iterator<isConstType>::operator--(int) {
        current_element_ = current_element_->prev_;
        return Iterator(current_element_->next_);
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    bool List<T, Tag>::Iterator<isConstType>::operator==(const Iterator& other) const noexcept {
        return current_element_ == other.current_element_;
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    bool List<T, Tag>::Iterator<isConstType>::operator!=(const Iterator& other) const noexcept {
        return current_element_ != other.current_element_;
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    List<T, Tag>::Iterator<isConstType>::reference List<T, Tag>::Iterator<isConstType>::operator*() const {
        return *static_cast<T*>((static_cast<ListElement<Tag>*>(current_element_)));
    }

    template <typename T, typename Tag>
    requires IsListElement<T, Tag>
    template <bool isConstType>
    List<T, Tag>::Iterator<isConstType>::pointer List<T, Tag>::Iterator<isConstType>::operator->() const {
        return static_cast<T*>((static_cast<ListElement<Tag>*>(current_element_)));
    }


    template <typename T, typename Tag>
    void swap(List<T, Tag>& first, List<T, Tag>& second) {
        first.Swap(second);
    }

} //End of namespace intrusive

#endif //CPP_IMPLEMENTATIONS_INTRUSIVE_LIST_H
