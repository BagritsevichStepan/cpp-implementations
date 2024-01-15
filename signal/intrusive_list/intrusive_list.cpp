#include "intrusive_list.h"

namespace cpp::intrusive {

    ListElementBase::ListElementBase() : prev_(this), next_(this) {}

    void ListElementBase::InsertBetween(ListElementBase* left, ListElementBase* right) {
        prev_ = left;
        next_ = right;
        left->next_ = this;
        right->prev_ = this;
    }

    void ListElementBase::InsertBefore(ListElementBase& element) {
        InsertBetween(element.prev_, &element);
    }

    void ListElementBase::InsertAfter(ListElementBase& element) {
        InsertBetween(&element, element.next_);
    }

    void ListElementBase::Unlink() {
        prev_->next_ = next_;
        next_->prev_ = prev_;
        prev_ = next_ = this;
    }

    bool ListElementBase::IsLinked() const {
        return prev_ != this && next_ != this;
    }

    ListElementBase::~ListElementBase() {
        Unlink();
    }

} //End of namespace intrusive
