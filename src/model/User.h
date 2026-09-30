#pragma once

#include <string>

class User {
private:
    int id;
    std::string name;
    std::string department;

public:
    User(int id, const std::string &name, const std::string &department)
        : id(id), name(name), department(department) {}

    int getId() const {
        return id;
    }

    const std::string &getName() const {
        return name;
    }

    const std::string &getDepartment() const {
        return department;
    }

    void setName(const std::string &name) {
        this->name = name;
    }

    void setDepartment(const std::string &department) {
        this->department = department;
    }
};
