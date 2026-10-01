#include <cassert>
#include <iostream>
#include <string>

#include "datastruct/HashTable.h"
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

int main() {
    testHashTableBasic();
    testHashTableDuplicateKey();
    testHashTableRemove();
    testHashTableCollision();
    testHashTableStringKey();
    testHashTableClear();
    testHashTableForEach();
    testHashTableWithModelObjects();

    std::cout << "test_datastruct (HashTable): all passed" << std::endl;
    return 0;
}
