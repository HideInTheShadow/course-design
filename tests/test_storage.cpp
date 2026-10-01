#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "service/LabService.h"
#include "storage/DataStore.h"

// 测试依赖 assert，必须使用 Debug 构建（Release 下 NDEBUG 会禁用检查）

// 时间统一使用分钟粒度的 long long，预约区间为 [start, end)

namespace {

const char *roundTripFile = "test_storage_roundtrip.txt";
const char *brokenFile = "test_storage_broken.txt";
const char *missingFile = "test_storage_missing.txt";

int addUser(LabService &service, const std::string &name, const std::string &department) {
    int userId = 0;
    service.users().addUser(name, department, userId);
    return userId;
}

int addCategory(LabService &service, int parentId, const std::string &name) {
    int nodeId = -1;
    service.devices().addCategory(parentId, name, nodeId);
    return nodeId;
}

int addDevice(LabService &service, const std::string &name, const std::string &category) {
    int deviceId = 0;
    service.devices().addDevice(name, category, "A301", deviceId);
    return deviceId;
}

}

void testRoundTrip() {
    remove(roundTripFile);

    LabService service;
    int firstUser = addUser(service, "张三", "计算机学院");
    int secondUser = addUser(service, "李四", "电子工程学院");

    int rootCategory = addCategory(service, -1, "电子测量");
    int childCategory = addCategory(service, rootCategory, "示波器类");

    int firstDevice = addDevice(service, "示波器 A", "示波器类");
    int secondDevice = addDevice(service, "信号发生器", "电子测量");

    int activeReservation = 0;
    int waitingReservation = 0;
    assert(service.reserve(firstUser, firstDevice, 600, 720, activeReservation) == OpResult::Ok);
    assert(service.waitReservation(secondUser, firstDevice, 660, 780, waitingReservation) == OpResult::Ok);

    int openRecord = 0;
    assert(service.borrowDevice(firstDevice, firstUser, 630, openRecord) == OpResult::Ok);
    assert(service.returnDevice(firstDevice, 700) == OpResult::Ok);

    DataStore store(roundTripFile);
    assert(store.save(service));

    LabService loaded;
    DataStore loadedStore(roundTripFile);
    assert(loadedStore.load(loaded) > 0);
    assert(loadedStore.skippedLines() == 0);

    // 用户：编号、姓名、部门
    assert(loaded.users().userCount() == 2);
    User user(0, "", "");
    assert(loaded.users().findUser(firstUser, user));
    assert(user.getName() == "张三");
    assert(user.getDepartment() == "计算机学院");

    // 分类树：父子关系按下标原样恢复
    assert(loaded.devices().categoryCount() == 2);
    assert(loaded.devices().findCategory("电子测量") == rootCategory);
    assert(loaded.devices().findCategory("示波器类") == childCategory);

    // 设备：分类名与状态
    assert(loaded.devices().deviceCount() == 2);
    Device device(0, "", "", "");
    assert(loaded.devices().findDevice(firstDevice, device));
    assert(device.getName() == "示波器 A");
    assert(device.getCategory() == "示波器类");
    assert(device.getLocation() == "A301");
    assert(device.getStatus() == DeviceStatus::Reserved);  // 已归还，但仍有有效预约
    assert(loaded.devices().findDevice(secondDevice, device));
    assert(device.getStatus() == DeviceStatus::Idle);

    // 预约：编号、时间与状态
    assert(loaded.reservations().reservationCount() == 2);
    Reservation reservation(0, 0, 0, 0, 0);
    assert(loaded.reservations().findReservation(activeReservation, reservation));
    assert(reservation.getUserId() == firstUser);
    assert(reservation.getDeviceId() == firstDevice);
    assert(reservation.getStartTime() == 600);
    assert(reservation.getEndTime() == 720);
    assert(reservation.getStatus() == ReservationStatus::Active);

    // 等待队列：排队次序按文件恢复
    std::vector<int> waitingIds;
    loaded.reservations().forEachWaiting([&waitingIds](const Reservation &waiting) {
        waitingIds.push_back(waiting.getId());
    });
    assert(waitingIds.size() == 1);
    assert(waitingIds[0] == waitingReservation);
    assert(loaded.reservations().waitingCount() == 1);

    // 借用记录：编号、借用与归还时间
    std::vector<int> borrowIds;
    loaded.forEachBorrowRecord([&borrowIds](const BorrowRecord &each) {
        borrowIds.push_back(each.getId());
    });
    assert(borrowIds.size() == 1);
    assert(borrowIds[0] == openRecord);

    BorrowRecord record(0, 0, 0, 0);
    assert(loaded.findOpenBorrow(firstDevice, record) == false);  // 已归还
    bool foundReturned = false;
    loaded.forEachBorrowRecord([&foundReturned](const BorrowRecord &each) {
        if (each.isReturned() && each.getBorrowTime() == 630 && each.getReturnTime() == 700) {
            foundReturned = true;
        }
    });
    assert(foundReturned);

    // 编号计数已经推进：新数据不会与文件中已有的编号冲突
    int newUser = 0;
    assert(loaded.users().addUser("王五", "计算机学院", newUser) == OpResult::Ok);
    assert(newUser > secondUser);
    assert(loaded.users().userCount() == 3);

    int newReservation = 0;
    assert(loaded.reserve(secondUser, secondDevice, 600, 720, newReservation) == OpResult::Ok);
    assert(newReservation > waitingReservation);
}

void testSkipBrokenLines() {
    std::ofstream output(brokenFile, std::ios::trunc);
    output << "# 故意写坏的数据文件\n";
    output << "[用户]\n";
    output << "1|张三|计算机学院\n";
    output << "1|重复编号|计算机学院\n";      // 编号重复
    output << "2|字段太少\n";                 // 字段个数不对
    output << "0|编号非法|计算机学院\n";       // 编号非正
    output << "[分类]\n";
    output << "0|-1|电子测量\n";
    output << "1|7|父节点不存在\n";            // 父下标越界，且下标与扫描顺序不一致
    output << "[设备]\n";
    output << "1|示波器|电子测量|0|A301\n";
    output << "2|坏状态设备|电子测量|9|A301\n";  // 状态码越界
    output << "[预约]\n";
    output << "1|1|1|600|720|0\n";
    output << "2|1|1|800|700|0\n";              // 开始时间晚于结束时间
    output << "3|1|9|600|720|0\n";              // 设备不存在
    output << "4|1|1|600|720|7\n";              // 状态码越界
    output << "[等待队列]\n";
    output << "4\n";                            // 预约不存在
    output << "[借用记录]\n";
    output << "1|1|1|600|700|1\n";
    output << "2|1|1|700|600|1\n";              // 归还时间早于借用时间
    output.close();

    LabService service;
    DataStore store(brokenFile);
    int loadedCount = store.load(service);

    // 1 条用户 + 1 条分类 + 1 台设备 + 1 条预约 + 1 条借用记录
    assert(loadedCount == 5);
    assert(store.skippedLines() == 10);
    assert(service.users().userCount() == 1);
    assert(service.devices().categoryCount() == 1);
    assert(service.devices().deviceCount() == 1);
    assert(service.reservations().reservationCount() == 1);
    assert(service.borrowRecordCount() == 1);
    assert(service.reservations().waitingCount() == 0);

    // 坏行没有破坏已经载入的好数据
    Device device(0, "", "", "");
    assert(service.devices().findDevice(1, device));
    assert(device.getStatus() == DeviceStatus::Idle);
}

void testMissingFileAndEmptyData() {
    remove(missingFile);

    LabService service;
    DataStore store(missingFile);
    assert(store.load(service) == 0);
    assert(store.skippedLines() == 0);   // 文件不存在不算坏行

    // 空数据也能保存并读回，不会留下半截文件
    assert(store.save(service));
    assert(store.load(service) == 0);
    assert(service.users().userCount() == 0);
    assert(service.devices().deviceCount() == 0);
    assert(service.reservations().reservationCount() == 0);
    assert(service.borrowRecordCount() == 0);
}

int main() {
    testRoundTrip();
    testSkipBrokenLines();
    testMissingFileAndEmptyData();

    std::cout << "test_storage: all tests passed" << std::endl;
    return 0;
}
