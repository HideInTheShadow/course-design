#pragma once

// 顺序栈：动态数组实现，容量不足时按 2 倍扩容，均摊 O(1)
// 元素类型需可默认构造并支持拷贝赋值
template<typename T>
class Stack {
private:
    T *data;
    int capacity;
    int count;

    void grow() {
        int newCapacity = capacity * 2;
        T *newData = new T[newCapacity];
        for (int i = 0; i < count; ++i) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

public:
    explicit Stack(int capacity = 16)
        : capacity(capacity > 0 ? capacity : 1), count(0) {
        data = new T[this->capacity];
    }

    ~Stack() {
        delete[] data;
    }

    Stack(const Stack &) = delete;
    Stack &operator=(const Stack &) = delete;

    void push(const T &value) {
        if (count == capacity) grow();

        data[count] = value;
        ++count;
    }

    // 弹出栈顶元素，空栈返回 false
    bool pop() {
        if (count == 0) return false;

        --count;
        return true;
    }

    // 查看栈顶元素但不弹出，空栈返回 false
    bool top(T &out) const {
        if (count == 0) return false;

        out = data[count - 1];
        return true;
    }

    bool empty() const {
        return count == 0;
    }

    int size() const {
        return count;
    }

    void clear() {
        count = 0;
    }
};
