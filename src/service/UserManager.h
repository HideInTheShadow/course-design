#pragma once

#include <functional>
#include <string>

#include "datastruct/HashTable.h"
#include "model/User.h"
#include "service/OpResult.h"

// 用户管理：编号到 User 对象的哈希表，编号由内部自增分配
class UserManager {
private:
    HashTable<int, User> users;
    int nextId;

public:
    UserManager() : nextId(1) {}

    // 新增用户，成功时通过 newId 返回分配的编号
    OpResult addUser(const std::string &name, const std::string &department, int &newId) {
        if (name.empty() || department.empty()) return OpResult::InvalidInput;

        newId = nextId++;
        users.insert(newId, User(newId, name, department));
        return OpResult::Ok;
    }

    // 已删除的编号不再复用，避免历史记录指向新用户
    OpResult removeUser(int userId) {
        if (!users.remove(userId)) return OpResult::NotFound;

        return OpResult::Ok;
    }

    OpResult renameUser(int userId, const std::string &name) {
        if (name.empty()) return OpResult::InvalidInput;
        if (!users.contains(userId)) return OpResult::NotFound;

        User user(0, "", "");  // 仅作接收容器
        users.find(userId, user);
        user.setName(name);
        users.update(userId, user);
        return OpResult::Ok;
    }

    OpResult setDepartment(int userId, const std::string &department) {
        if (department.empty()) return OpResult::InvalidInput;
        if (!users.contains(userId)) return OpResult::NotFound;

        User user(0, "", "");  // 仅作接收容器
        users.find(userId, user);
        user.setDepartment(department);
        users.update(userId, user);
        return OpResult::Ok;
    }

    // 载入数据文件：按文件中记录的编号原样写回，并把编号计数推进到安全位置
    // 与 addUser 的区别是不重新分配编号，因此不做重名校验，只拒绝非法与重复编号
    bool restore(const User &user) {
        if (user.getId() <= 0 || user.getName().empty() || user.getDepartment().empty()) return false;
        if (users.contains(user.getId())) return false;

        users.insert(user.getId(), user);
        if (user.getId() >= nextId) nextId = user.getId() + 1;
        return true;
    }

    bool findUser(int userId, User &out) const {
        return users.find(userId, out);
    }

    bool contains(int userId) const {
        return users.contains(userId);
    }

    int userCount() const {
        return users.size();
    }

    // 供列表展示使用
    void forEach(const std::function<void(const User &)> &fn) const {
        users.forEach([&fn](const int &, const User &user) {
            fn(user);
        });
    }
};
