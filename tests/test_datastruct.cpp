#include <cassert>
#include <iostream>
#include <string>

#include "datastruct/HashTable.h"
#include "datastruct/LinkedList.h"
#include "datastruct/Queue.h"
#include "datastruct/Stack.h"
#include "datastruct/Tree.h"
#include "model/BorrowRecord.h"
#include "model/Device.h"
#include "model/Reservation.h"

// 测试依赖 assert，必须使用 Debug 构建（Release 下 NDEBUG 会禁用检查）

// 撤销栈的元素类型：记录一次与预约相关的可撤销操作
struct ActionRecord {
    int type;  // 1 新建预约，2 取消预约
    int reservationId;
};

void testHashTableBasic() {
    HashTable<int, std::string> table;

    assert(table.size() == 0);
    assert(!table.contains(1));

    assert(table.insert(1, "one"));
    assert(table.insert(2, "two"));
    assert(table.insert(3, "three"));
    assert(table.size() == 3);

    std::string value;
    assert(table.find(2, value));
    assert(value == "two");

    assert(!table.find(99, value));
}

void testHashTableDuplicateKey() {
    HashTable<int, std::string> table;

    assert(table.insert(1, "first"));
    assert(!table.insert(1, "second"));
    assert(table.size() == 1);

    std::string value;
    assert(table.find(1, value));
    assert(value == "first");
}

void testHashTableRemove() {
    HashTable<int, int> table;
    for (int i = 1; i <= 100; ++i) {
        table.insert(i, i * 10);
    }
    assert(table.size() == 100);

    assert(table.remove(50));
    assert(!table.contains(50));
    assert(table.size() == 99);

    assert(!table.remove(50));   // 重复删除
    assert(!table.remove(999));  // 不存在的键

    // 删除非头结点，验证前驱指针重连
    assert(table.remove(99));
    assert(!table.contains(99));
    assert(table.size() == 98);

    // 剩余元素完好
    int value = 0;
    assert(table.find(1, value) && value == 10);
    assert(table.find(100, value) && value == 1000);
}

void testHashTableUpdate() {
    HashTable<int, std::string> table;
    table.insert(1, "old");

    assert(table.update(1, "new"));
    assert(table.size() == 1);

    std::string value;
    assert(table.find(1, value));
    assert(value == "new");

    // 键不存在时不新增
    assert(!table.update(2, "x"));
    assert(table.size() == 1);

    // 更新模型对象
    HashTable<int, Device> devices;
    devices.insert(101, Device(101, "示波器", "电子测量", "A301"));
    assert(devices.update(101, Device(101, "示波器", "电子测量", "B502")));

    Device probe(0, "", "", "");  // 仅作接收容器
    assert(devices.find(101, probe));
    assert(probe.getLocation() == "B502");
}

void testHashTableCollision() {
    // 桶数取 7，插入 100 个键必然产生大量冲突
    HashTable<int, int> table(7);
    for (int i = 0; i < 100; ++i) {
        table.insert(i, i * i);
    }
    assert(table.size() == 100);

    int value = 0;
    for (int i = 0; i < 100; ++i) {
        assert(table.find(i, value));
        assert(value == i * i);
    }

    // 0、7、14 落在同一桶，验证链式冲突处理
    assert(table.contains(0) && table.contains(7) && table.contains(14));
    assert(table.remove(7));
    assert(table.contains(0) && !table.contains(7) && table.contains(14));
}

void testHashTableStringKey() {
    HashTable<std::string, int> table;
    table.insert("user001", 1);
    table.insert("user002", 2);
    table.insert("device001", 11);

    int value = 0;
    assert(table.find("device001", value));
    assert(value == 11);
    assert(!table.find("nobody", value));
    assert(table.remove("user002"));
    assert(table.size() == 2);
}

void testHashTableClear() {
    HashTable<int, int> table;
    for (int i = 0; i < 10; ++i) {
        table.insert(i, i);
    }
    assert(table.size() == 10);

    table.clear();
    assert(table.size() == 0);
    assert(!table.contains(5));

    // 清空后仍可继续使用
    assert(table.insert(5, 55));
    assert(table.size() == 1);
}

void testHashTableForEach() {
    HashTable<int, int> table;
    for (int i = 1; i <= 50; ++i) {
        table.insert(i, i);
    }

    int sum = 0;
    int visited = 0;
    table.forEach([&sum, &visited](const int &key, const int &value) {
        sum += value;
        ++visited;
    });

    assert(visited == 50);
    assert(sum == 50 * 51 / 2);
}

void testHashTableWithModelObjects() {
    HashTable<int, Device> devices;
    devices.insert(101, Device(101, "示波器", "电子测量", "A301"));
    devices.insert(102, Device(102, "万用表", "电子测量", "A302"));

    Device probe(0, "", "", "");  // 仅作接收容器
    assert(devices.find(101, probe));
    assert(probe.getName() == "示波器");
    assert(probe.getStatus() == DeviceStatus::Idle);

    assert(devices.remove(101));
    assert(!devices.find(101, probe));
    assert(devices.size() == 1);
}

void testLinkedListPush() {
    LinkedList<int> list;

    assert(list.empty());
    assert(list.size() == 0);

    for (int i = 1; i <= 5; ++i) {
        list.pushBack(i);
    }
    assert(list.size() == 5);

    // pushBack 保持插入顺序
    int expectedBack[] = {1, 2, 3, 4, 5};
    int index = 0;
    list.forEach([&expectedBack, &index](const int &value) {
        assert(value == expectedBack[index]);
        ++index;
    });
    assert(index == 5);

    // pushFront 逆序
    LinkedList<int> front;
    for (int i = 1; i <= 5; ++i) {
        front.pushFront(i);
    }
    int expectedFront[] = {5, 4, 3, 2, 1};
    index = 0;
    front.forEach([&expectedFront, &index](const int &value) {
        assert(value == expectedFront[index]);
        ++index;
    });
    assert(index == 5);
}

void testLinkedListEmptyEdge() {
    LinkedList<int> list;

    assert(!list.removeIf([](const int &value) { return value == 1; }));
    assert(list.size() == 0);

    int visited = 0;
    list.forEach([&visited](const int &value) { ++visited; });
    assert(visited == 0);

    list.clear();
    assert(list.empty());
}

void testLinkedListRemove() {
    LinkedList<int> list;
    for (int i = 1; i <= 5; ++i) {
        list.pushBack(i);
    }

    // 删除中间节点
    assert(list.removeIf([](const int &value) { return value == 3; }));
    assert(list.size() == 4);
    assert(!list.removeIf([](const int &value) { return value == 3; }));

    // 删除头节点
    assert(list.removeIf([](const int &value) { return value == 1; }));
    assert(list.size() == 3);

    // 删除尾节点后继续 pushBack，验证 tail 指针已正确回退
    assert(list.removeIf([](const int &value) { return value == 5; }));
    assert(list.size() == 2);

    list.pushBack(6);
    int expected[] = {2, 4, 6};
    int index = 0;
    list.forEach([&expected, &index](const int &value) {
        assert(value == expected[index]);
        ++index;
    });
    assert(index == 3);

    // 逐个删空
    assert(list.removeIf([](const int &value) { return value == 2; }));
    assert(list.removeIf([](const int &value) { return value == 4; }));
    assert(list.removeIf([](const int &value) { return value == 6; }));
    assert(list.empty());
    assert(list.size() == 0);
}

void testLinkedListClear() {
    LinkedList<int> list;
    for (int i = 0; i < 100; ++i) {
        list.pushBack(i);
    }
    assert(list.size() == 100);

    list.clear();
    assert(list.empty());
    assert(list.size() == 0);

    // 清空后可复用
    list.pushBack(42);
    assert(list.size() == 1);
}

void testLinkedListWithBorrowRecords() {
    LinkedList<BorrowRecord> history;
    history.pushBack(BorrowRecord(1, 101, 1, 600));
    history.pushBack(BorrowRecord(2, 101, 2, 700));
    history.pushBack(BorrowRecord(3, 102, 1, 800));

    int device101Count = 0;
    history.forEach([&device101Count](const BorrowRecord &record) {
        if (record.getDeviceId() == 101) ++device101Count;
    });
    assert(device101Count == 2);

    // 移除一条未归还的记录
    assert(history.removeIf([](const BorrowRecord &record) {
        return !record.isReturned();
    }));
    assert(history.size() == 2);
}

void testQueueBasic() {
    Queue<int> queue;

    assert(queue.empty());
    assert(queue.size() == 0);

    int value = 0;
    assert(!queue.front(value));
    assert(!queue.pop());

    for (int i = 1; i <= 5; ++i) {
        queue.push(i);
    }
    assert(queue.size() == 5);

    // 先进先出，front 不改变队列
    assert(queue.front(value) && value == 1);
    assert(queue.size() == 5);

    for (int i = 1; i <= 5; ++i) {
        assert(queue.front(value));
        assert(value == i);
        assert(queue.pop());
    }
    assert(queue.empty());
    assert(!queue.pop());
}

void testQueueReuse() {
    Queue<int> queue;

    // 反复填满再清空，验证队空后 tail 指针正确复位
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < 10; ++i) {
            queue.push(i);
        }
        while (queue.pop()) {
        }
        assert(queue.empty());
    }

    queue.push(99);
    int value = 0;
    assert(queue.front(value) && value == 99);
}

void testQueueClear() {
    Queue<int> queue;
    for (int i = 0; i < 100; ++i) {
        queue.push(i);
    }
    assert(queue.size() == 100);

    queue.clear();
    assert(queue.empty());
    assert(queue.size() == 0);

    queue.push(1);
    assert(queue.size() == 1);
}

void testQueueForEach() {
    Queue<int> queue;
    for (int i = 1; i <= 5; ++i) {
        queue.push(i);
    }

    // forEach 按出队顺序遍历且不改变队列
    int expected[] = {1, 2, 3, 4, 5};
    int index = 0;
    queue.forEach([&expected, &index](const int &value) {
        assert(value == expected[index]);
        ++index;
    });
    assert(index == 5);
    assert(queue.size() == 5);
}

void testQueueWithReservations() {
    // 等待队列场景：设备释放后按提交顺序依次处理
    Queue<Reservation> waiting;
    waiting.push(Reservation(1, 1, 101, 600, 720));
    waiting.push(Reservation(2, 2, 101, 720, 840));
    waiting.push(Reservation(3, 3, 101, 900, 960));
    assert(waiting.size() == 3);

    Reservation probe(0, 0, 0, 0, 0);  // 仅作接收容器
    assert(waiting.front(probe));
    assert(probe.getId() == 1);
    assert(probe.getDeviceId() == 101);

    assert(waiting.pop());
    assert(waiting.front(probe));
    assert(probe.getId() == 2);
    assert(waiting.size() == 2);
}

void testStackBasic() {
    Stack<int> stack;

    assert(stack.empty());
    assert(stack.size() == 0);

    int value = 0;
    assert(!stack.top(value));
    assert(!stack.pop());

    for (int i = 1; i <= 5; ++i) {
        stack.push(i);
    }
    assert(stack.size() == 5);

    // 后进先出，top 只读不弹出
    assert(stack.top(value) && value == 5);
    assert(stack.size() == 5);

    for (int i = 5; i >= 1; --i) {
        assert(stack.top(value));
        assert(value == i);
        assert(stack.pop());
    }
    assert(stack.empty());
    assert(!stack.pop());
}

void testStackGrow() {
    // 初始容量取 4，插入 100 个元素触发多次 2 倍扩容
    Stack<int> stack(4);
    for (int i = 1; i <= 100; ++i) {
        stack.push(i);
    }
    assert(stack.size() == 100);

    // 扩容搬迁后顺序不丢，仍为后进先出
    int value = 0;
    for (int i = 100; i >= 1; --i) {
        assert(stack.top(value));
        assert(value == i);
        assert(stack.pop());
    }
    assert(stack.empty());
}

void testStackClear() {
    Stack<int> stack;
    for (int i = 0; i < 50; ++i) {
        stack.push(i);
    }
    assert(stack.size() == 50);

    stack.clear();
    assert(stack.empty());
    assert(stack.size() == 0);

    stack.push(7);
    assert(stack.size() == 1);
}

void testStackWithActionRecords() {
    // 撤销栈：最近一次操作在栈顶，撤销时先弹出
    Stack<ActionRecord> actions;
    actions.push(ActionRecord{1, 101});
    actions.push(ActionRecord{1, 102});
    actions.push(ActionRecord{2, 101});
    assert(actions.size() == 3);

    ActionRecord last{0, 0};  // 仅作接收容器
    assert(actions.top(last));
    assert(last.type == 2);
    assert(last.reservationId == 101);
    assert(actions.pop());

    assert(actions.top(last));
    assert(last.type == 1);
    assert(last.reservationId == 102);
    assert(actions.size() == 2);
}

void testTreeAddAndFind() {
    Tree<std::string> tree;
    assert(tree.empty());
    assert(tree.size() == 0);

    int root = tree.addRoot("电子测量");
    int scope = tree.addChild(root, "示波器");
    int meter = tree.addChild(root, "万用表");
    int analog = tree.addChild(scope, "模拟示波器");
    assert(tree.size() == 4);

    // 父链与兄弟链
    assert(tree.parentOf(root) == -1);
    assert(tree.firstChildOf(root) == scope);
    assert(tree.nextSiblingOf(scope) == meter);
    assert(tree.nextSiblingOf(meter) == -1);
    assert(tree.parentOf(analog) == scope);
    assert(tree.firstChildOf(analog) == -1);

    // 非法父下标
    assert(tree.addChild(-1, "非法") == -1);
    assert(tree.addChild(99, "非法") == -1);
    assert(tree.size() == 4);

    // 查找
    assert(tree.find("示波器") == scope);
    assert(tree.find("不存在") == -1);
    assert(tree.findChild(root, "万用表") == meter);
    assert(tree.findChild(root, "模拟示波器") == -1);  // 不是直接孩子
    assert(tree.findChild(scope, "模拟示波器") == analog);

    std::string value;
    assert(tree.valueAt(analog, value));
    assert(value == "模拟示波器");
    assert(!tree.valueAt(99, value));
}

void testTreeDuplicateNameAllowed() {
    Tree<std::string> tree;
    int rootA = tree.addRoot("电子测量");
    int rootB = tree.addRoot("机械加工");
    int childA = tree.addChild(rootA, "传感器");
    int childB = tree.addChild(rootB, "传感器");

    // 同名节点可以存在于不同父节点下，全局唯一性不在此处强制
    assert(tree.find("传感器") == childA);
    assert(tree.findChild(rootA, "传感器") == childA);
    assert(tree.findChild(rootB, "传感器") == childB);
}

void testTreeTraverse() {
    Tree<std::string> tree;
    int rootA = tree.addRoot("A");
    int nodeB = tree.addChild(rootA, "B");
    tree.addChild(rootA, "C");
    tree.addChild(nodeB, "D");
    tree.addRoot("E");

    std::string order[5];
    int visited = 0;
    tree.traversePreOrder([&order, &visited, &tree](const std::string &value, int nodeId) {
        assert(nodeId >= 0 && nodeId < tree.size());
        order[visited++] = value;
    });

    // 前序：A B D C E
    assert(visited == 5);
    assert(order[0] == "A");
    assert(order[1] == "B");
    assert(order[2] == "D");
    assert(order[3] == "C");
    assert(order[4] == "E");
}

void testTreeGrow() {
    // 初始容量取 4，插入 100 个节点触发多次 2 倍扩容
    Tree<std::string> tree(4);
    int root = tree.addRoot("root");
    for (int i = 0; i < 100; ++i) {
        tree.addChild(root, "child" + std::to_string(i));
    }
    assert(tree.size() == 101);

    // 扩容搬迁后下标关系仍在，孩子链顺序保持添加顺序
    int child = tree.firstChildOf(root);
    for (int i = 0; i < 100; ++i) {
        std::string value;
        assert(tree.valueAt(child, value));
        assert(value == "child" + std::to_string(i));
        child = tree.nextSiblingOf(child);
    }
    assert(child == -1);
}

void testTreeRemoveLeaf() {
    Tree<std::string> tree;
    int root = tree.addRoot("root");
    tree.addChild(root, "A");
    int nodeB = tree.addChild(root, "B");
    tree.addChild(root, "C");

    // 删除中间的孩子，验证前驱指针重连
    assert(tree.remove(nodeB));

    std::string order[3];
    int visited = 0;
    tree.traversePreOrder([&order, &visited](const std::string &value, int) {
        order[visited++] = value;
    });
    assert(visited == 3);
    assert(order[0] == "root");
    assert(order[1] == "A");
    assert(order[2] == "C");

    assert(tree.find("B") == -1);
    assert(!tree.remove(-1));
    assert(!tree.remove(99));
}

void testTreeRemoveSubtree() {
    Tree<std::string> tree;
    int root = tree.addRoot("root");
    int nodeA = tree.addChild(root, "A");
    int nodeA1 = tree.addChild(nodeA, "A1");
    tree.addChild(nodeA1, "A1x");
    tree.addChild(root, "B");
    assert(tree.size() == 5);

    // 删除 A 及其整棵子树
    assert(tree.remove(nodeA));
    assert(tree.size() == 2);
    assert(tree.find("A") == -1);
    assert(tree.find("A1") == -1);
    assert(tree.find("A1x") == -1);

    std::string order[2];
    int visited = 0;
    tree.traversePreOrder([&order, &visited](const std::string &value, int) {
        order[visited++] = value;
    });
    assert(visited == 2);
    assert(order[0] == "root");
    assert(order[1] == "B");

    // 删除后下标已重新编号：root 为 0，B 为 1
    assert(tree.parentOf(0) == -1);
    assert(tree.firstChildOf(0) == 1);
    assert(tree.parentOf(1) == 0);

    // 删除根，整棵树清空
    assert(tree.remove(0));
    assert(tree.empty());
    assert(tree.size() == 0);
}

void testTreeClear() {
    Tree<std::string> tree;
    int root = tree.addRoot("root");
    tree.addChild(root, "A");
    assert(tree.size() == 2);

    tree.clear();
    assert(tree.empty());
    assert(tree.size() == 0);

    // 清空后可复用
    int newRoot = tree.addRoot("new");
    assert(newRoot == 0);
    assert(tree.size() == 1);
}

int main() {
    testHashTableBasic();
    testHashTableDuplicateKey();
    testHashTableUpdate();
    testHashTableRemove();
    testHashTableCollision();
    testHashTableStringKey();
    testHashTableClear();
    testHashTableForEach();
    testHashTableWithModelObjects();

    testLinkedListPush();
    testLinkedListEmptyEdge();
    testLinkedListRemove();
    testLinkedListClear();
    testLinkedListWithBorrowRecords();

    testQueueBasic();
    testQueueReuse();
    testQueueClear();
    testQueueForEach();
    testQueueWithReservations();

    testStackBasic();
    testStackGrow();
    testStackClear();
    testStackWithActionRecords();

    testTreeAddAndFind();
    testTreeDuplicateNameAllowed();
    testTreeTraverse();
    testTreeGrow();
    testTreeRemoveLeaf();
    testTreeRemoveSubtree();
    testTreeClear();

    std::cout << "test_datastruct: all passed" << std::endl;
    return 0;
}
