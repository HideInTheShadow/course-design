#pragma once

#include <functional>
#include <string>

// 整数键直接取模，字符串键用 BKDR 哈希
template<typename K>
struct KeyHash {
    unsigned int operator()(const K &key) const {
        return static_cast<unsigned int>(key);
    }
};

template<>
struct KeyHash<std::string> {
    unsigned int operator()(const std::string &key) const {
        unsigned int hash = 0;
        for (char c : key) {
            hash = hash * 131 + static_cast<unsigned char>(c);
        }
        return hash;
    }
};

// 链地址法：桶数组 + 每桶一条单链表，冲突元素挂在同一桶上
template<typename K, typename V>
class HashTable {
private:
    struct Node {
        K key;
        V value;
        Node *next;
    };

    Node **buckets;
    int bucketCount;
    int count;

    unsigned int hashKey(const K &key) const {
        return KeyHash<K>()(key) % static_cast<unsigned int>(bucketCount);
    }

    void freeAll() {
        for (int i = 0; i < bucketCount; ++i) {
            Node *cur = buckets[i];
            while (cur) {
                Node *next = cur->next;
                delete cur;
                cur = next;
            }
            buckets[i] = nullptr;
        }
        count = 0;
    }

public:
    explicit HashTable(int bucketCount = 1021)
        : bucketCount(bucketCount > 0 ? bucketCount : 1), count(0) {
        buckets = new Node *[this->bucketCount];
        for (int i = 0; i < this->bucketCount; ++i) {
            buckets[i] = nullptr;
        }
    }

    ~HashTable() {
        freeAll();
        delete[] buckets;
    }

    // 节点由裸指针持有，禁止拷贝，避免二次释放
    HashTable(const HashTable &) = delete;
    HashTable &operator=(const HashTable &) = delete;

    bool insert(const K &key, const V &value) {
        unsigned int index = hashKey(key);
        for (Node *cur = buckets[index]; cur; cur = cur->next) {
            if (cur->key == key) return false;
        }

        // 聚合初始化直接拷贝构造，不要求 K/V 可默认构造
        Node *node = new Node{key, value, buckets[index]};
        buckets[index] = node;
        ++count;
        return true;
    }

    bool find(const K &key, V &out) const {
        unsigned int index = hashKey(key);
        for (Node *cur = buckets[index]; cur; cur = cur->next) {
            if (cur->key == key) {
                out = cur->value;
                return true;
            }
        }
        return false;
    }

    bool remove(const K &key) {
        unsigned int index = hashKey(key);
        Node *prev = nullptr;
        for (Node *cur = buckets[index]; cur; cur = cur->next) {
            if (cur->key == key) {
                if (prev) prev->next = cur->next;
                else buckets[index] = cur->next;
                delete cur;
                --count;
                return true;
            }
            prev = cur;
        }
        return false;
    }

    bool contains(const K &key) const {
        unsigned int index = hashKey(key);
        for (Node *cur = buckets[index]; cur; cur = cur->next) {
            if (cur->key == key) return true;
        }
        return false;
    }

    int size() const {
        return count;
    }

    void clear() {
        freeAll();
    }

    void forEach(const std::function<void(const K &, const V &)> &fn) const {
        for (int i = 0; i < bucketCount; ++i) {
            for (Node *cur = buckets[i]; cur; cur = cur->next) {
                fn(cur->key, cur->value);
            }
        }
    }
};
