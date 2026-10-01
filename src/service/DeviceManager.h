#pragma once

#include <functional>
#include <string>

#include "datastruct/HashTable.h"
#include "datastruct/Tree.h"
#include "model/Device.h"
#include "service/OpResult.h"

// 设备管理：编号到 Device 对象的哈希表 + 多级分类树
// 设备只记录分类名，层级关系由分类树维护，删除分类不会牵连 Device 对象本身
class DeviceManager {
private:
    HashTable<int, Device> devices;
    Tree<std::string> categories;
    int nextId;

    // 在同一父节点下查找同名分类；parentId 为 -1 时在所有顶层分类中查找
    bool categoryNameExists(int parentId, const std::string &name) const {
        if (parentId != -1) return categories.findChild(parentId, name) != -1;

        std::string value;
        for (int i = 0; i < categories.size(); ++i) {
            if (categories.parentOf(i) == -1 && categories.valueAt(i, value) && value == name) {
                return true;
            }
        }
        return false;
    }

    // 分类 nodeId 或它的任一后代是否已被设备引用
    bool categoryInUse(int nodeId) const {
        bool used = false;
        devices.forEach([this, nodeId, &used](const int &, const Device &device) {
            if (used) return;

            int node = categories.find(device.getCategory());
            while (node != -1) {
                if (node == nodeId) {
                    used = true;
                    return;
                }
                node = categories.parentOf(node);
            }
        });
        return used;
    }

public:
    DeviceManager() : nextId(1) {}

    // 设备：新增，成功时通过 newId 返回分配的编号
    // 分类必须已存在，避免设备挂到不存在的分类上
    OpResult addDevice(const std::string &name, const std::string &category,
                       const std::string &location, int &newId) {
        if (name.empty() || category.empty() || location.empty()) return OpResult::InvalidInput;
        if (categories.find(category) == -1) return OpResult::NotFound;

        newId = nextId++;
        devices.insert(newId, Device(newId, name, category, location));
        return OpResult::Ok;
    }

    // 已借出或维修中的设备不允许直接删除
    OpResult removeDevice(int deviceId) {
        Device device(0, "", "", "");  // 仅作接收容器
        if (!devices.find(deviceId, device)) return OpResult::NotFound;
        if (device.getStatus() == DeviceStatus::Borrowed) return OpResult::InvalidState;

        devices.remove(deviceId);
        return OpResult::Ok;
    }

    OpResult updateDevice(int deviceId, const std::string &name, const std::string &category,
                          const std::string &location) {
        if (name.empty() || category.empty() || location.empty()) return OpResult::InvalidInput;
        if (categories.find(category) == -1) return OpResult::NotFound;

        Device device(0, "", "", "");  // 仅作接收容器
        if (!devices.find(deviceId, device)) return OpResult::NotFound;

        device.setName(name);
        device.setCategory(category);
        device.setLocation(location);
        devices.update(deviceId, device);
        return OpResult::Ok;
    }

    OpResult setStatus(int deviceId, DeviceStatus status) {
        Device device(0, "", "", "");  // 仅作接收容器
        if (!devices.find(deviceId, device)) return OpResult::NotFound;

        device.setStatus(status);
        devices.update(deviceId, device);
        return OpResult::Ok;
    }

    bool findDevice(int deviceId, Device &out) const {
        return devices.find(deviceId, out);
    }

    bool contains(int deviceId) const {
        return devices.contains(deviceId);
    }

    int deviceCount() const {
        return devices.size();
    }

    // 供设备列表展示使用
    void forEachDevice(const std::function<void(const Device &)> &fn) const {
        devices.forEach([&fn](const int &, const Device &device) {
            fn(device);
        });
    }

    // 分类树：新建分类，parentId 传 -1 表示顶层分类
    OpResult addCategory(int parentId, const std::string &name, int &newNodeId) {
        if (name.empty()) return OpResult::InvalidInput;
        if (parentId < -1 || parentId >= categories.size()) return OpResult::NotFound;
        if (categoryNameExists(parentId, name)) return OpResult::Duplicate;

        if (parentId == -1) newNodeId = categories.addRoot(name);
        else newNodeId = categories.addChild(parentId, name);
        return OpResult::Ok;
    }

    // 分类或其子分类仍被设备使用时拒绝删除
    OpResult removeCategory(int nodeId) {
        if (nodeId < 0 || nodeId >= categories.size()) return OpResult::NotFound;
        if (categoryInUse(nodeId)) return OpResult::InvalidState;

        categories.remove(nodeId);
        return OpResult::Ok;
    }

    int findCategory(const std::string &name) const {
        return categories.find(name);
    }

    int categoryCount() const {
        return categories.size();
    }

    // 前序输出分类，fn 参数依次为分类名、节点下标、父节点下标（顶层分类的父下标为 -1）
    void forEachCategory(const std::function<void(const std::string &, int, int)> &fn) const {
        categories.traversePreOrder([&fn, this](const std::string &name, int nodeId) {
            fn(name, nodeId, categories.parentOf(nodeId));
        });
    }
};
