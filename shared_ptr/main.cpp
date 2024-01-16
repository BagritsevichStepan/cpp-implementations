#include <iostream>
#include <cassert>
#include "shared_ptr.h"

namespace {

    class EmptyClass {
    public:
        EmptyClass() {
            std::cout << "EmptyClass ctor()" << std::endl;
        }

        ~EmptyClass() {
            std::cout << "EmptyClass dtor()" << std::endl;
        }

    };

}

int main() {
    cpp::pointer::SharedPointer<EmptyClass> shared_ptr{new EmptyClass};
    auto shared_ptr2 = shared_ptr;
    auto shared_ptr3 = shared_ptr2;

    assert(shared_ptr.UseCount() == 3);
    assert(shared_ptr2.UseCount() == 3);
    assert(shared_ptr3.UseCount() == 3);

    cpp::pointer::SharedPointer<EmptyClass> inplace_shared_ptr = cpp::pointer::MakeShared<EmptyClass>();
    shared_ptr = inplace_shared_ptr;
    auto inplace_shared_ptr2 = inplace_shared_ptr;

    assert(shared_ptr.UseCount() == 3);
    assert(shared_ptr2.UseCount() == 2);
    assert(shared_ptr3.UseCount() == 2);
    assert(inplace_shared_ptr.UseCount() == 3);
    assert(inplace_shared_ptr.UseCount() == 3);


    cpp::pointer::WeakPointer<EmptyClass>* weak_pointer1;
    {
        cpp::pointer::SharedPointer<EmptyClass> shared_ptr_for_weak = cpp::pointer::MakeShared<EmptyClass>();
        weak_pointer1 = new cpp::pointer::WeakPointer(shared_ptr_for_weak);
    }

    assert(weak_pointer1->IsExpired());
    assert(weak_pointer1->UseCount() == 0);


    cpp::pointer::WeakPointer<EmptyClass>* weak_pointer2;
    {
        cpp::pointer::SharedPointer<EmptyClass> shared_ptr_for_weak{new EmptyClass};
        weak_pointer2 = new cpp::pointer::WeakPointer(shared_ptr_for_weak);
    }

    assert(weak_pointer2->IsExpired());
    assert(weak_pointer2->UseCount() == 0);

    return 0;
}
