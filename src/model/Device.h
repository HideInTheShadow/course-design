#pragma once

#include <string>

enum class DeviceStatus {
    Idle,      // 空闲
    Reserved,  // 已预约
    Borrowed,  // 已借出
    Repair     // 维修中
};

class Device {
private:
    int id;
    std::string name;
    std::string category;  // 分类名，层级关系由 DeviceManager 的分类树维护
    DeviceStatus status;
    std::string location;

public:
    Device(int id, const std::string &name, const std::string &category,
           const std::string &location)
        : id(id), name(name), category(category),
          status(DeviceStatus::Idle), location(location) {}

    int getId() const {
        return id;
    }

    const std::string &getName() const {
        return name;
    }

    const std::string &getCategory() const {
        return category;
    }

    DeviceStatus getStatus() const {
        return status;
    }

    const std::string &getLocation() const {
        return location;
    }

    void setName(const std::string &name) {
        this->name = name;
    }

    void setCategory(const std::string &category) {
        this->category = category;
    }

    void setStatus(DeviceStatus status) {
        this->status = status;
    }

    void setLocation(const std::string &location) {
        this->location = location;
    }
};
