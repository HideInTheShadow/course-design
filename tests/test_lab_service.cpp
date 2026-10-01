#include <cassert>
#include <iostream>
#include <string>

#include "service/LabService.h"

// 测试依赖 assert，必须使用 Debug 构建（Release 下 NDEBUG 会禁用检查）

// 时间统一使用分钟粒度的 long long，预约区间为 [start, end)

int addUser(LabService &service, const std::string &name) {
    int userId = 0;
    service.users().addUser(name, "计算机学院", userId);
    return userId;
}

int addDevice(LabService &service, const std::string &name) {
    int categoryId = -1;
    if (service.devices().findCategory("电子测量") == -1) {
        service.devices().addCategory(-1, "电子测量", categoryId);
    }

    int deviceId = 0;
    service.devices().addDevice(name, "电子测量", "A301", deviceId);
    return deviceId;
}

void testReserveFlow() {
    LabService service;
    int userId = addUser(service, "张三");
    int deviceId = addDevice(service, "示波器");

    int reservationId = 0;
    assert(service.reserve(userId, deviceId, 600, 720, reservationId) == OpResult::Ok);

    Device device(0, "", "", "");  // 仅作接收容器
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Reserved);

    // 时间冲突被拒绝，设备状态不变
    int rejected = -1;
    assert(service.reserve(userId, deviceId, 660, 780, rejected) == OpResult::TimeConflict);
    assert(service.reservations().reservationCount() == 1);

    // 不存在的用户或设备
    assert(service.reserve(99, deviceId, 600, 720, rejected) == OpResult::NotFound);
    assert(service.reserve(userId, 99, 600, 720, rejected) == OpResult::NotFound);

    // 半开区间：首尾相接不算冲突
    int second = 0;
    assert(service.reserve(userId, deviceId, 720, 840, second) == OpResult::Ok);

    // 预约全部取消后设备回到空闲
    assert(service.cancelReservation(reservationId) == OpResult::Ok);
    assert(service.cancelReservation(second) == OpResult::Ok);
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Idle);
}

void testWaitFlow() {
    LabService service;
    int firstUser = addUser(service, "张三");
    int secondUser = addUser(service, "李四");
    int deviceId = addDevice(service, "示波器");

    int active = 0;
    assert(service.reserve(firstUser, deviceId, 600, 720, active) == OpResult::Ok);

    int waiting = 0;
    assert(service.waitReservation(secondUser, deviceId, 620, 700, waiting) == OpResult::Ok);
    assert(service.reservations().waitingCount() == 1);

    Reservation reservation(0, 0, 0, 0, 0);
    assert(service.reservations().findReservation(waiting, reservation));
    assert(reservation.getStatus() == ReservationStatus::Waiting);

    // 对方取消后时段释放，等待任务转为有效预约
    assert(service.cancelReservation(active) == OpResult::Ok);
    assert(service.reservations().waitingCount() == 0);
    assert(service.reservations().findReservation(waiting, reservation));
    assert(reservation.getStatus() == ReservationStatus::Active);

    Device device(0, "", "", "");
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Reserved);

    // 已无冲突时选择排队，直接按有效预约建立
    int direct = 0;
    assert(service.waitReservation(firstUser, deviceId, 900, 960, direct) == OpResult::Ok);
    assert(service.reservations().waitingCount() == 0);
    assert(service.reservations().findReservation(direct, reservation));
    assert(reservation.getStatus() == ReservationStatus::Active);

    // 用户或设备不存在时不建立等待任务
    assert(service.waitReservation(99, deviceId, 900, 960, direct) == OpResult::NotFound);
    assert(service.waitReservation(firstUser, 99, 900, 960, direct) == OpResult::NotFound);
}

void testBorrowReturnFlow() {
    LabService service;
    int userId = addUser(service, "张三");
    int otherUser = addUser(service, "李四");
    int deviceId = addDevice(service, "示波器");

    int reservationId = 0;
    assert(service.reserve(userId, deviceId, 600, 720, reservationId) == OpResult::Ok);

    // 没有预约的用户不能借用
    int recordId = 0;
    assert(service.borrowDevice(deviceId, otherUser, 650, recordId) == OpResult::InvalidState);

    // 预约区间之外不能借用，半开区间 [600, 720) 的终点同样越界
    assert(service.borrowDevice(deviceId, userId, 800, recordId) == OpResult::InvalidState);
    assert(service.borrowDevice(deviceId, userId, 720, recordId) == OpResult::InvalidState);

    // 区间起点可以借用
    assert(service.borrowDevice(deviceId, userId, 600, recordId) == OpResult::Ok);

    Device device(0, "", "", "");
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Borrowed);
    assert(service.borrowRecordCount() == 1);

    BorrowRecord record(0, 0, 0, 0);
    assert(service.findOpenBorrow(deviceId, record));
    assert(record.getDeviceId() == deviceId);
    assert(record.getUserId() == userId);
    assert(!record.isReturned());

    // 借出期间不能重复借用
    assert(service.borrowDevice(deviceId, userId, 650, recordId) == OpResult::InvalidState);

    // 归还后仍有一条有效预约，设备进入「已预约」
    assert(service.returnDevice(deviceId, 700) == OpResult::Ok);
    assert(service.returnDevice(deviceId, 700) == OpResult::InvalidState);  // 重复归还
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Reserved);
    assert(!service.findOpenBorrow(deviceId, record));

    // 取消预约后设备回到空闲
    assert(service.cancelReservation(reservationId) == OpResult::Ok);
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Idle);

    // 借用历史保留完整记录，已归还的仍然可查
    int visited = 0;
    service.forEachBorrowRecord([&visited](const BorrowRecord &item) {
        assert(item.getId() > 0);
        ++visited;
    });
    assert(visited == 1);
}

void testReturnKeepsConflictingWaiting() {
    LabService service;
    int firstUser = addUser(service, "张三");
    int secondUser = addUser(service, "李四");
    int deviceId = addDevice(service, "示波器");

    int active = 0;
    int waiting = 0;
    service.reserve(firstUser, deviceId, 600, 720, active);
    service.waitReservation(secondUser, deviceId, 700, 800, waiting);

    // 借出再归还不会提升仍然冲突的等待任务
    int recordId = 0;
    assert(service.borrowDevice(deviceId, firstUser, 650, recordId) == OpResult::Ok);
    assert(service.returnDevice(deviceId, 700) == OpResult::Ok);
    assert(service.reservations().waitingCount() == 1);

    // 时段真正释放后才生效
    assert(service.cancelReservation(active) == OpResult::Ok);
    assert(service.reservations().waitingCount() == 0);

    Reservation reservation(0, 0, 0, 0, 0);
    assert(service.reservations().findReservation(waiting, reservation));
    assert(reservation.getStatus() == ReservationStatus::Active);
}

void testUndoCancel() {
    LabService service;
    int userId = addUser(service, "张三");
    int rivalUser = addUser(service, "李四");
    int deviceId = addDevice(service, "示波器");

    int reservationId = 0;
    service.reserve(userId, deviceId, 600, 720, reservationId);

    int restoredId = 0;
    assert(service.undoDepth() == 0);
    assert(service.undoLastCancel(restoredId) == OpResult::NotFound);  // 没有可撤销的操作

    assert(service.cancelReservation(reservationId) == OpResult::Ok);
    assert(service.undoDepth() == 1);
    assert(service.undoLastCancel(restoredId) == OpResult::Ok);
    assert(restoredId == reservationId);
    assert(service.undoDepth() == 0);

    Reservation reservation(0, 0, 0, 0, 0);
    assert(service.reservations().findReservation(reservationId, reservation));
    assert(reservation.getStatus() == ReservationStatus::Active);

    Device device(0, "", "", "");
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Reserved);

    // 时段已被别人占用时撤销失败，操作记录保留以便稍后再试
    assert(service.cancelReservation(reservationId) == OpResult::Ok);

    int rival = 0;
    assert(service.reserve(rivalUser, deviceId, 600, 720, rival) == OpResult::Ok);
    assert(service.undoLastCancel(restoredId) == OpResult::TimeConflict);
    assert(service.undoDepth() == 1);
}

void testRemoveConstraints() {
    LabService service;
    int userId = addUser(service, "张三");
    int deviceId = addDevice(service, "示波器");

    int reservationId = 0;
    service.reserve(userId, deviceId, 600, 720, reservationId);

    // 有未取消预约的用户与设备都不能删除
    assert(service.removeUser(userId) == OpResult::InvalidState);
    assert(service.removeDevice(deviceId) == OpResult::InvalidState);

    // 取消预约后可以删除
    assert(service.cancelReservation(reservationId) == OpResult::Ok);
    assert(service.removeDevice(deviceId) == OpResult::Ok);
    assert(service.removeUser(userId) == OpResult::Ok);
    assert(service.devices().deviceCount() == 0);
    assert(service.users().userCount() == 0);
}

void testRemoveUserWithOpenBorrow() {
    LabService service;
    int userId = addUser(service, "张三");
    int deviceId = addDevice(service, "示波器");

    int reservationId = 0;
    service.reserve(userId, deviceId, 600, 720, reservationId);

    int recordId = 0;
    assert(service.borrowDevice(deviceId, userId, 650, recordId) == OpResult::Ok);
    assert(service.cancelReservation(reservationId) == OpResult::Ok);

    // 设备还没归还，用户不能删除
    assert(service.removeUser(userId) == OpResult::InvalidState);

    assert(service.returnDevice(deviceId, 700) == OpResult::Ok);
    assert(service.removeUser(userId) == OpResult::Ok);
}

void testRepairStatus() {
    LabService service;
    int userId = addUser(service, "张三");
    int deviceId = addDevice(service, "示波器");

    Device device(0, "", "", "");  // 仅作接收容器

    // 有未取消预约时不能标记维修
    int reservationId = 0;
    assert(service.reserve(userId, deviceId, 600, 720, reservationId) == OpResult::Ok);
    assert(service.markDeviceRepair(deviceId) == OpResult::InvalidState);

    // 取消预约后可以标记，维修中的设备不再接受预约与排队
    assert(service.cancelReservation(reservationId) == OpResult::Ok);
    assert(service.markDeviceRepair(deviceId) == OpResult::Ok);
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Repair);

    int rejected = 0;
    assert(service.reserve(userId, deviceId, 600, 720, rejected) == OpResult::InvalidState);
    assert(service.waitReservation(userId, deviceId, 600, 720, rejected) == OpResult::InvalidState);

    // 重复标记与对正常设备结束维修都不合法
    assert(service.markDeviceRepair(deviceId) == OpResult::InvalidState);
    assert(service.clearDeviceRepair(999) == OpResult::NotFound);

    // 维修结束：没有有效预约则回到空闲，之后可以正常预约
    assert(service.clearDeviceRepair(deviceId) == OpResult::Ok);
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Idle);
    assert(service.reserve(userId, deviceId, 600, 720, rejected) == OpResult::Ok);

    // 已经有有效预约的设备同样不能标记维修
    assert(service.markDeviceRepair(deviceId) == OpResult::InvalidState);
    assert(service.cancelReservation(rejected) == OpResult::Ok);
    assert(service.markDeviceRepair(deviceId) == OpResult::Ok);
    assert(service.clearDeviceRepair(deviceId) == OpResult::Ok);
    assert(service.devices().findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Idle);
}

void testBorrowRepairDevice() {
    LabService service;
    int userId = addUser(service, "张三");
    int deviceId = addDevice(service, "示波器");

    assert(service.markDeviceRepair(deviceId) == OpResult::Ok);

    // 维修中的设备不能借用，也不能归还（没有借出记录）
    int recordId = 0;
    assert(service.borrowDevice(deviceId, userId, 650, recordId) == OpResult::InvalidState);
    assert(service.returnDevice(deviceId, 700) == OpResult::InvalidState);
}

int main() {
    testReserveFlow();
    testWaitFlow();
    testBorrowReturnFlow();
    testReturnKeepsConflictingWaiting();
    testUndoCancel();
    testRemoveConstraints();
    testRemoveUserWithOpenBorrow();
    testRepairStatus();
    testBorrowRepairDevice();

    std::cout << "test_lab_service: all passed" << std::endl;
    return 0;
}
