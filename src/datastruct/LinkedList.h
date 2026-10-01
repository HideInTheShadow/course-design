#pragma once

#include <functional>

// 单向链表，维护 head 与 tail 指针，两端插入均为 O(1)
template<typename T>
class LinkedList {
private:
    struct Node {
        T value;
        Node *next;
    };

    Node *head;
    Node *tail;
    int count;

public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}

    ~LinkedList() {
        clear();
    }

    LinkedList(const LinkedList &) = delete;
    LinkedList &operator=(const LinkedList &) = delete;

    void pushFront(const T &value) {
        Node *node = new Node{value, head};
        head = node;
        if (!tail) tail = node;
        ++count;
    }

    void pushBack(const T &value) {
        Node *node = new Node{value, nullptr};
        if (tail) {
            tail->next = node;
        }
        else {
            head = node;
        }
        tail = node;
        ++count;
    }

    bool empty() const {
        return count == 0;
    }

    int size() const {
        return count;
    }

    void clear() {
        Node *cur = head;
        while (cur) {
            Node *next = cur->next;
            delete cur;
            cur = next;
        }
        head = nullptr;
        tail = nullptr;
        count = 0;
    }

    void forEach(const std::function<void(const T &)> &fn) const {
        for (Node *cur = head; cur; cur = cur->next) {
            fn(cur->value);
        }
    }

    // 删除第一个满足条件的节点，空表或无匹配时返回 false
    bool removeIf(const std::function<bool(const T &)> &pred) {
        Node *prev = nullptr;
        for (Node *cur = head; cur; cur = cur->next) {
            if (pred(cur->value)) {
                if (prev) prev->next = cur->next;
                else head = cur->next;
                if (cur == tail) tail = prev;
                delete cur;
                --count;
                return true;
            }
            prev = cur;
        }
        return false;
    }
};
