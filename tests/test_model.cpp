#include <cassert>
#include <iostream>

#include "model/BorrowRecord.h"
#include "model/Device.h"
#include "model/Reservation.h"
#include "model/User.h"

void testUser() {
    User user(1, "张三", "计算机学院");

    assert(user.getId() == 1);
    assert(user.getName() == "张三");
    assert(user.getDepartment() == "计算机学院");

    user.setName("李四");
    assert(user.getName() == "李四");
}

void testDevice() {
    Device device(101, "示波器", "电子测量", "A301");

    assert(device.getId() == 101);
    assert(device.getStatus() == DeviceStatus::Idle);

    device.setStatus(DeviceStatus::Borrowed);
    device.setLocation("A302");
    assert(device.getStatus() == DeviceStatus::Borrowed);
    assert(device.getLocation() == "A302");
}

void testReservationConflict() {
    // 10:00-12:00
    Reservation morning(1, 1, 101, 600, 720);

    // 12:00-14:00 首尾相接，按半开区间规则不算冲突
    Reservation noon(2, 2, 101, 720, 840);
    assert(!morning.conflictsWith(noon));
    assert(!noon.conflictsWith(morning));

    // 11:00-13:00 跨越边界，冲突
    Reservation overlap(3, 3, 101, 660, 780);
    assert(morning.conflictsWith(overlap));
    assert(overlap.conflictsWith(morning));

    // 10:30-11:30 被完全包含，冲突
    Reservation inner(4, 4, 101, 630, 690);
    assert(morning.conflictsWith(inner));
    assert(inner.conflictsWith(morning));

    // 15:00-16:00 完全不相交
    Reservation later(5, 5, 101, 900, 960);
    assert(!morning.conflictsWith(later));
    assert(!later.conflictsWith(morning));
}

void testReservationTimeRange() {
    Reservation reversed(1, 1, 101, 720, 600);
    assert(!reversed.isValidTimeRange());

    Reservation zeroLength(2, 2, 101, 600, 600);
    assert(!zeroLength.isValidTimeRange());

    Reservation valid(3, 3, 101, 600, 720);
    assert(valid.isValidTimeRange());
}

void testBorrowRecord() {
    BorrowRecord record(1, 101, 1, 600);

    assert(record.getStatus() == BorrowStatus::Borrowed);
    assert(!record.isReturned());
    assert(record.getReturnTime() == 0);

    record.markReturned(720);
    assert(record.getStatus() == BorrowStatus::Returned);
    assert(record.isReturned());
    assert(record.getReturnTime() == 720);
}

int main() {
    testUser();
    testDevice();
    testReservationConflict();
    testReservationTimeRange();
    testBorrowRecord();

    std::cout << "test_model: all passed" << std::endl;
    return 0;
}
