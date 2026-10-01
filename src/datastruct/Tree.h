#pragma once

#include <functional>

// 孩子兄弟表示法：每个节点只记录「第一个孩子」和「下一个兄弟」两条链，
// 就能表示任意多个孩子，不必为每个节点维护不定长的孩子数组。
// 节点集中存放在动态数组里，父子关系用下标而非指针表示，数组扩容不会产生悬空指针。
// 注意：remove() 会重建数组并重新编号，删除后旧下标全部失效，调用方需重新遍历获取。
template<typename T>
class Tree {
public:
    struct Node {
        T value;
        int parent;       // 父节点下标，根为 -1
        int firstChild;   // 第一个孩子下标，无孩子为 -1
        int nextSibling;  // 下一个兄弟下标，无兄弟为 -1
    };

private:
    Node *nodes;
    int capacity;
    int count;

    void grow() {
        int newCapacity = capacity * 2;
        Node *newNodes = new Node[newCapacity];
        for (int i = 0; i < count; ++i) {
            newNodes[i] = nodes[i];
        }
        delete[] nodes;
        nodes = newNodes;
        capacity = newCapacity;
    }

    int newNode(const T &value, int parentId) {
        if (count == capacity) grow();

        int self = count++;
        nodes[self].value = value;
        nodes[self].parent = parentId;
        nodes[self].firstChild = -1;
        nodes[self].nextSibling = -1;
        return self;
    }

    // 把 childId 从 parentId 的孩子链上摘除，保持其余孩子的相对顺序
    void unlinkChild(int parentId, int childId) {
        int prev = -1;
        for (int cur = nodes[parentId].firstChild; cur != -1; cur = nodes[cur].nextSibling) {
            if (cur == childId) {
                if (prev == -1) nodes[parentId].firstChild = nodes[cur].nextSibling;
                else nodes[prev].nextSibling = nodes[cur].nextSibling;
                return;
            }
            prev = cur;
        }
    }

    // 前序遍历 oldId 的整棵子树，按新下标依次写入 newNodes
    void appendSubtree(int oldId, int newParent, Node *newNodes, int &newCount) {
        int self = newCount++;
        newNodes[self].value = nodes[oldId].value;
        newNodes[self].parent = newParent;
        newNodes[self].firstChild = -1;
        newNodes[self].nextSibling = -1;

        int prevChild = -1;
        for (int child = nodes[oldId].firstChild; child != -1; child = nodes[child].nextSibling) {
            int newChild = newCount;
            appendSubtree(child, self, newNodes, newCount);
            if (prevChild == -1) newNodes[self].firstChild = newChild;
            else newNodes[prevChild].nextSibling = newChild;
            prevChild = newChild;
        }
    }

    void traverseFrom(int nodeId, const std::function<void(const T &, int)> &fn) const {
        fn(nodes[nodeId].value, nodeId);
        for (int child = nodes[nodeId].firstChild; child != -1; child = nodes[child].nextSibling) {
            traverseFrom(child, fn);
        }
    }

public:
    explicit Tree(int capacity = 16)
        : capacity(capacity > 0 ? capacity : 1), count(0) {
        nodes = new Node[this->capacity];
    }

    ~Tree() {
        delete[] nodes;
    }

    Tree(const Tree &) = delete;
    Tree &operator=(const Tree &) = delete;

    // 追加一个根节点，返回其下标
    int addRoot(const T &value) {
        return newNode(value, -1);
    }

    // 在 parentId 下追加孩子，父下标非法返回 -1
    // 挂在孩子链尾部以保持添加顺序，代价 O(兄弟数)
    int addChild(int parentId, const T &value) {
        if (parentId < 0 || parentId >= count) return -1;

        int self = newNode(value, parentId);
        if (nodes[parentId].firstChild == -1) {
            nodes[parentId].firstChild = self;
        }
        else {
            int last = nodes[parentId].firstChild;
            while (nodes[last].nextSibling != -1) {
                last = nodes[last].nextSibling;
            }
            nodes[last].nextSibling = self;
        }
        return self;
    }

    // 删除节点及其整棵子树，删除后所有下标重新编号，代价 O(n)
    bool remove(int nodeId) {
        if (nodeId < 0 || nodeId >= count) return false;

        if (nodes[nodeId].parent != -1) {
            unlinkChild(nodes[nodeId].parent, nodeId);
        }

        Node *newNodes = new Node[capacity];
        int newCount = 0;
        for (int i = 0; i < count; ++i) {
            // nodeId 已脱离父节点链；若它本身是根，则由 i != nodeId 排除
            if (i != nodeId && nodes[i].parent == -1) {
                appendSubtree(i, -1, newNodes, newCount);
            }
        }

        delete[] nodes;
        nodes = newNodes;
        count = newCount;
        return true;
    }

    // 返回首个值相同的节点下标，未找到返回 -1
    int find(const T &value) const {
        for (int i = 0; i < count; ++i) {
            if (nodes[i].value == value) return i;
        }
        return -1;
    }

    // 只在 parentId 的直接孩子中查找，用于同级重名校验
    int findChild(int parentId, const T &value) const {
        if (parentId < 0 || parentId >= count) return -1;

        for (int child = nodes[parentId].firstChild; child != -1; child = nodes[child].nextSibling) {
            if (nodes[child].value == value) return child;
        }
        return -1;
    }

    bool valueAt(int nodeId, T &out) const {
        if (nodeId < 0 || nodeId >= count) return false;

        out = nodes[nodeId].value;
        return true;
    }

    int parentOf(int nodeId) const {
        if (nodeId < 0 || nodeId >= count) return -1;

        return nodes[nodeId].parent;
    }

    int firstChildOf(int nodeId) const {
        if (nodeId < 0 || nodeId >= count) return -1;

        return nodes[nodeId].firstChild;
    }

    int nextSiblingOf(int nodeId) const {
        if (nodeId < 0 || nodeId >= count) return -1;

        return nodes[nodeId].nextSibling;
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

    // 前序遍历整片森林；fn 的第二个参数是节点下标
    void traversePreOrder(const std::function<void(const T &, int)> &fn) const {
        for (int i = 0; i < count; ++i) {
            if (nodes[i].parent == -1) {
                traverseFrom(i, fn);
            }
        }
    }
};
