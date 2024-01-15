#include <iostream>
#include "intrusive_list.h"

class NodeTag;

struct Node : public cpp::intrusive::ListElement<NodeTag> {
public:
    explicit Node(int value) : value_(value) {}

    friend std::ostream& operator<<(std::ostream& os, const Node& node) {
        return os << "Node=[value=" << node.value_ << "]";
    }

    int value_;
};

int main() {
    cpp::intrusive::List<Node, NodeTag> list;

    Node node1 = Node{1};
    Node node2 = Node{2};
    Node node3 = Node{3};
    Node node4 = Node{4};
    Node node5 = Node{5};

    list.PushBack(node1);
    list.PushFront(node2);
    list.PushBack(node3);
    list.PushFront(node4);
    list.PushBack(node5);

    for (int pos = 4; pos >= 0; pos--) {
        auto it = list.begin();
        for (int i = 0; i < pos; i++) {
            it++;
        }

        Node node = Node{pos * 10};

        auto inserted_it = list.Insert(it, node);
        std::cout << "Insert " << node << ": " << list << std::endl;

        list.Erase(inserted_it);
        std::cout << "Erase " << node << ": " << list << std::endl << std::endl;
    }

    std::cout << "Removing:" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << list << std::endl;
        if (i % 2) {
            list.PopFront();
        } else {
            list.PopBack();
        }
    }
    std::cout << list << std::endl;
    return 0;
}
