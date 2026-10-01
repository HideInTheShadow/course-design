#include <cassert>
#include <iostream>
#include <string>

#include "service/UserManager.h"

// 测试依赖 assert，必须使用 Debug 构建（Release 下 NDEBUG 会禁用检查）

void testAddUser() {
    UserManager manager;
    assert(manager.userCount() == 0);

    int firstId = 0;
    int secondId = 0;
    assert(manager.addUser("张三", "计算机学院", firstId) == OpResult::Ok);
    assert(manager.addUser("李四", "电子学院", secondId) == OpResult::Ok);
    assert(firstId == 1);
    assert(secondId == 2);
    assert(manager.userCount() == 2);

    // 空输入被拒绝，且不占用编号
    int rejectedId = -1;
    assert(manager.addUser("", "计算机学院", rejectedId) == OpResult::InvalidInput);
    assert(manager.addUser("王五", "", rejectedId) == OpResult::InvalidInput);
    assert(manager.addUser("", "", rejectedId) == OpResult::InvalidInput);
    assert(manager.userCount() == 2);

    int thirdId = 0;
    assert(manager.addUser("王五", "机械学院", thirdId) == OpResult::Ok);
    assert(thirdId == 3);
}

void testFindUser() {
    UserManager manager;
    int firstId = 0;
    manager.addUser("张三", "计算机学院", firstId);

    User user(0, "", "");  // 仅作接收容器
    assert(manager.findUser(firstId, user));
    assert(user.getId() == firstId);
    assert(user.getName() == "张三");
    assert(user.getDepartment() == "计算机学院");

    assert(!manager.findUser(99, user));
    assert(manager.contains(firstId));
    assert(!manager.contains(99));
}

void testRenameUser() {
    UserManager manager;
    int userId = 0;
    manager.addUser("张三", "计算机学院", userId);

    assert(manager.renameUser(userId, "张三丰") == OpResult::Ok);

    User user(0, "", "");
    assert(manager.findUser(userId, user));
    assert(user.getName() == "张三丰");
    assert(user.getDepartment() == "计算机学院");

    assert(manager.renameUser(userId, "") == OpResult::InvalidInput);
    assert(manager.renameUser(99, "无名") == OpResult::NotFound);
}

void testSetDepartment() {
    UserManager manager;
    int userId = 0;
    manager.addUser("张三", "计算机学院", userId);

    assert(manager.setDepartment(userId, "软件学院") == OpResult::Ok);

    User user(0, "", "");
    assert(manager.findUser(userId, user));
    assert(user.getDepartment() == "软件学院");
    assert(user.getName() == "张三");

    assert(manager.setDepartment(userId, "") == OpResult::InvalidInput);
    assert(manager.setDepartment(99, "软件学院") == OpResult::NotFound);
}

void testRemoveUser() {
    UserManager manager;
    int firstId = 0;
    int secondId = 0;
    manager.addUser("张三", "计算机学院", firstId);
    manager.addUser("李四", "电子学院", secondId);

    assert(manager.removeUser(firstId) == OpResult::Ok);
    assert(manager.removeUser(firstId) == OpResult::NotFound);  // 重复删除
    assert(manager.removeUser(99) == OpResult::NotFound);
    assert(manager.userCount() == 1);
    assert(!manager.contains(firstId));

    // 编号只增不减，删除后不回收
    int thirdId = 0;
    assert(manager.addUser("王五", "机械学院", thirdId) == OpResult::Ok);
    assert(thirdId == 3);
}

void testForEachUsers() {
    UserManager manager;
    int userId = 0;
    manager.addUser("张三", "计算机学院", userId);
    manager.addUser("李四", "电子学院", userId);
    manager.addUser("王五", "机械学院", userId);

    int visited = 0;
    int departmentTotal = 0;
    manager.forEach([&visited, &departmentTotal](const User &user) {
        ++visited;
        assert(user.getId() > 0);
        departmentTotal += static_cast<int>(user.getDepartment().size());
    });

    assert(visited == 3);
    assert(departmentTotal > 0);
}

int main() {
    testAddUser();
    testFindUser();
    testRenameUser();
    testSetDepartment();
    testRemoveUser();
    testForEachUsers();

    std::cout << "test_service: all passed" << std::endl;
    return 0;
}
