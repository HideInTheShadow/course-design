#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "service/LabService.h"

// 数据持久化：把内存中的数据结构写入本地文本文件，启动时再读回来
//
// 文件采用「分节 + 竖线分隔」的文本格式，一行一条记录，可以直接打开查看：
//   [用户]      编号|姓名|部门
//   [分类]      节点下标|父节点下标|分类名     父节点下标一定小于子节点，载入时可一次重建
//   [设备]      编号|名称|分类|状态|位置
//   [预约]      编号|用户编号|设备编号|开始时间|结束时间|状态
//   [等待队列]  预约编号                      按排队先后各占一行，顺序即优先级
//   [借用记录]  编号|设备编号|用户编号|借用时间|归还时间|状态
//
// 载入时字段个数不对、数值非法、编号重复或引用到不存在的用户与设备，都会跳过该条记录，
// 并计入 skippedLines()，避免损坏的数据进入核心数据结构
class DataStore {
private:
    std::string path;
    int skippedCount;

    static void trim(std::string &text) {
        std::size_t begin = text.find_first_not_of(" \t\r\n");
        std::size_t end = text.find_last_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            text.clear();
            return;
        }
        text = text.substr(begin, end - begin + 1);
    }

    static void split(const std::string &line, std::vector<std::string> &fields) {
        fields.clear();

        std::istringstream stream(line);
        std::string field;
        while (std::getline(stream, field, '|')) {
            trim(field);
            fields.push_back(field);
        }
    }

    static bool toInt(const std::string &text, int &value) {
        if (text.empty()) return false;

        std::istringstream stream(text);
        stream >> value;
        return stream && stream.eof();
    }

    static bool toTime(const std::string &text, long long &value) {
        if (text.empty()) return false;

        std::istringstream stream(text);
        stream >> value;
        return stream && stream.eof();
    }

    static int codeOf(DeviceStatus status) {
        switch (status) {
            case DeviceStatus::Idle: return 0;
            case DeviceStatus::Reserved: return 1;
            case DeviceStatus::Borrowed: return 2;
            case DeviceStatus::Repair: return 3;
        }
        return 0;
    }

    static int codeOf(ReservationStatus status) {
        switch (status) {
            case ReservationStatus::Active: return 0;
            case ReservationStatus::Cancelled: return 1;
            case ReservationStatus::Waiting: return 2;
        }
        return 0;
    }

    static int codeOf(BorrowStatus status) {
        return status == BorrowStatus::Returned ? 1 : 0;
    }

    static bool deviceStatusOf(int code, DeviceStatus &status) {
        if (code < 0 || code > 3) return false;

        status = static_cast<DeviceStatus>(code);
        return true;
    }

    static bool reservationStatusOf(int code, ReservationStatus &status) {
        if (code < 0 || code > 2) return false;

        status = static_cast<ReservationStatus>(code);
        return true;
    }

    bool loadUser(LabService &service, const std::vector<std::string> &fields) {
        if (fields.size() != 3) return false;

        int userId = 0;
        if (!toInt(fields[0], userId)) return false;

        return service.users().restore(User(userId, fields[1], fields[2]));
    }

    bool loadCategory(LabService &service, const std::vector<std::string> &fields) {
        if (fields.size() != 3) return false;

        // 文件按下标从小到大保存，父节点一定先于子节点出现，
        // 因此按顺序重建出来的下标应当与文件中的下标一致，不一致说明文件被改动过
        int nodeId = 0;
        int parentId = 0;
        if (!toInt(fields[0], nodeId)) return false;
        if (!toInt(fields[1], parentId)) return false;

        int newNodeId = service.devices().restoreCategory(parentId, fields[2]);
        return newNodeId != -1 && newNodeId == nodeId;
    }

    bool loadDevice(LabService &service, const std::vector<std::string> &fields) {
        if (fields.size() != 5) return false;

        int deviceId = 0;
        int statusCode = 0;
        DeviceStatus status = DeviceStatus::Idle;
        if (!toInt(fields[0], deviceId)) return false;
        if (!toInt(fields[3], statusCode)) return false;
        if (!deviceStatusOf(statusCode, status)) return false;

        Device device(deviceId, fields[1], fields[2], fields[4]);
        device.setStatus(status);
        return service.devices().restore(device);
    }

    bool loadReservation(LabService &service, const std::vector<std::string> &fields) {
        if (fields.size() != 6) return false;

        int reservationId = 0;
        int userId = 0;
        int deviceId = 0;
        long long startTime = 0;
        long long endTime = 0;
        int statusCode = 0;
        ReservationStatus status = ReservationStatus::Active;
        if (!toInt(fields[0], reservationId)) return false;
        if (!toInt(fields[1], userId)) return false;
        if (!toInt(fields[2], deviceId)) return false;
        if (!toTime(fields[3], startTime)) return false;
        if (!toTime(fields[4], endTime)) return false;
        if (!toInt(fields[5], statusCode)) return false;
        if (!reservationStatusOf(statusCode, status)) return false;

        // 引用到已经不存在的用户或设备，说明文件与数据不一致
        if (!service.users().contains(userId)) return false;
        if (!service.devices().contains(deviceId)) return false;

        return service.restoreReservation(Reservation(reservationId, userId, deviceId,
                                                      startTime, endTime, status));
    }

    bool loadWaitingOrder(LabService &service, const std::vector<std::string> &fields) {
        if (fields.size() != 1) return false;

        int reservationId = 0;
        if (!toInt(fields[0], reservationId)) return false;
        if (!service.reservations().contains(reservationId)) return false;

        service.restoreWaitingOrder(reservationId);
        return true;
    }

    bool loadBorrowRecord(LabService &service, const std::vector<std::string> &fields) {
        if (fields.size() != 6) return false;

        int recordId = 0;
        int deviceId = 0;
        int userId = 0;
        long long borrowTime = 0;
        long long returnTime = 0;
        int statusCode = 0;
        if (!toInt(fields[0], recordId)) return false;
        if (!toInt(fields[1], deviceId)) return false;
        if (!toInt(fields[2], userId)) return false;
        if (!toTime(fields[3], borrowTime)) return false;
        if (!toTime(fields[4], returnTime)) return false;
        if (!toInt(fields[5], statusCode)) return false;
        if (statusCode < 0 || statusCode > 1) return false;

        if (borrowTime < 0) return false;
        if (!service.users().contains(userId)) return false;
        if (!service.devices().contains(deviceId)) return false;

        BorrowRecord record(recordId, deviceId, userId, borrowTime);
        if (statusCode == 1) {
            if (returnTime < borrowTime) return false;
            record.markReturned(returnTime);
        }

        service.restoreBorrowRecord(record);
        return true;
    }

public:
    explicit DataStore(const std::string &path = "lab_data.txt")
        : path(path), skippedCount(0) {}

    // 载入数据文件，返回成功载入的记录条数；文件不存在时返回 0
    int load(LabService &service) {
        skippedCount = 0;

        std::ifstream input(path.c_str());
        if (!input) return 0;

        int loadedCount = 0;
        std::string section;
        std::string line;
        std::vector<std::string> fields;

        while (std::getline(input, line)) {
            trim(line);
            if (line.empty() || line[0] == '#') continue;

            if (line.front() == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                continue;
            }

            split(line, fields);

            bool loaded = false;
            if (section == "用户") loaded = loadUser(service, fields);
            else if (section == "分类") loaded = loadCategory(service, fields);
            else if (section == "设备") loaded = loadDevice(service, fields);
            else if (section == "预约") loaded = loadReservation(service, fields);
            else if (section == "等待队列") loaded = loadWaitingOrder(service, fields);
            else if (section == "借用记录") loaded = loadBorrowRecord(service, fields);

            if (loaded) ++loadedCount;
            else ++skippedCount;
        }
        return loadedCount;
    }

    // 保存全部数据；文件无法写入时返回 false，此时内存中的数据不受影响
    bool save(const LabService &service) const {
        std::ofstream output(path.c_str(), std::ios::trunc);
        if (!output) return false;

        output << "# 实验室共享设备预约与冲突调度系统数据文件\n";
        output << "# 一行一条记录，竖线分隔；格式错误的记录会在载入时被跳过\n";

        output << "\n[用户]\n";
        service.users().forEach([&output](const User &user) {
            output << user.getId() << '|' << user.getName() << '|' << user.getDepartment() << '\n';
        });

        output << "\n[分类]\n";
        service.devices().forEachCategoryByIndex([&output](const std::string &name, int nodeId,
                                                           int parentId) {
            output << nodeId << '|' << parentId << '|' << name << '\n';
        });

        output << "\n[设备]\n";
        service.devices().forEachDevice([&output](const Device &device) {
            output << device.getId() << '|' << device.getName() << '|' << device.getCategory()
                   << '|' << codeOf(device.getStatus()) << '|' << device.getLocation() << '\n';
        });

        output << "\n[预约]\n";
        service.reservations().forEachReservation([&output](const Reservation &reservation) {
            output << reservation.getId() << '|' << reservation.getUserId() << '|'
                   << reservation.getDeviceId() << '|' << reservation.getStartTime() << '|'
                   << reservation.getEndTime() << '|' << codeOf(reservation.getStatus()) << '\n';
        });

        output << "\n[等待队列]\n";
        service.reservations().forEachWaiting([&output](const Reservation &reservation) {
            output << reservation.getId() << '\n';
        });

        output << "\n[借用记录]\n";
        service.forEachBorrowRecord([&output](const BorrowRecord &record) {
            output << record.getId() << '|' << record.getDeviceId() << '|' << record.getUserId()
                   << '|' << record.getBorrowTime() << '|' << record.getReturnTime()
                   << '|' << codeOf(record.getStatus()) << '\n';
        });

        output.flush();
        return output.good();
    }

    int skippedLines() const {
        return skippedCount;
    }

    const std::string &filePath() const {
        return path;
    }
};
