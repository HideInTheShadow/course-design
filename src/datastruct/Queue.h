#pragma once

// 链式队列：head 端出队、tail 端入队，均为 O(1)
template<typename T>
class Queue {
private:
    struct Node {
        T value;
        Node *next;
    };

    Node *head;
    Node *tail;
    int count;

public:
    Queue() : head(nullptr), tail(nullptr), count(0) {}

    ~Queue() {
        clear();
    }

    Queue(const Queue &) = delete;
    Queue &operator=(const Queue &) = delete;

    void push(const T &value) {
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

    // 移除队首元素，空队列返回 false
    bool pop() {
        if (!head) return false;

        Node *node = head;
        head = head->next;
        if (!head) tail = nullptr;
        delete node;
        --count;
        return true;
    }

    // 查看队首元素但不移除，空队列返回 false
    bool front(T &out) const {
        if (!head) return false;

        out = head->value;
        return true;
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
};
