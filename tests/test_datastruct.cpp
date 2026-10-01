#include <cassert>
#include <iostream>
#include <string>

#include "datastruct/HashTable.h"
#include "datastruct/LinkedList.h"
#include "model/BorrowRecord.h"
#include "model/Device.h"

// 测试依赖 assert，必须使用 Debug 构建（Release 下 NDEBUG 会禁用检查）

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

int main() {
    testHashTableBasic();
    testHashTableDuplicateKey();
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

    std::cout << "test_datastruct: all passed" << std::endl;
    return 0;
}
