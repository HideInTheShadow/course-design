#include <cassert>
#include <iostream>
#include <string>

#include "service/DeviceManager.h"
#include "service/ReservationManager.h"
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

void testAddCategory() {
    DeviceManager manager;
    assert(manager.categoryCount() == 0);

    int electronics = -1;
    int mechanical = -1;
    int scope = -1;
    assert(manager.addCategory(-1, "电子测量", electronics) == OpResult::Ok);
    assert(manager.addCategory(-1, "机械加工", mechanical) == OpResult::Ok);
    assert(manager.addCategory(electronics, "示波器", scope) == OpResult::Ok);
    assert(manager.categoryCount() == 3);

    // 同一父节点下重名被拒绝
    int rejected = -1;
    assert(manager.addCategory(-1, "电子测量", rejected) == OpResult::Duplicate);
    assert(manager.addCategory(electronics, "示波器", rejected) == OpResult::Duplicate);
    assert(manager.categoryCount() == 3);

    // 不同父节点下同名允许
    int scopeUnderMechanical = -1;
    assert(manager.addCategory(mechanical, "示波器", scopeUnderMechanical) == OpResult::Ok);
    assert(scopeUnderMechanical != scope);

    // 非法输入
    assert(manager.addCategory(-1, "", rejected) == OpResult::InvalidInput);
    assert(manager.addCategory(99, "频谱仪", rejected) == OpResult::NotFound);
    assert(manager.addCategory(-2, "频谱仪", rejected) == OpResult::NotFound);

    assert(manager.findCategory("示波器") == scope);
    assert(manager.findCategory("频谱仪") == -1);
}

void testAddDevice() {
    DeviceManager manager;
    int electronics = -1;
    manager.addCategory(-1, "电子测量", electronics);

    int scopeId = 0;
    int meterId = 0;
    assert(manager.addDevice("示波器", "电子测量", "A301", scopeId) == OpResult::Ok);
    assert(manager.addDevice("万用表", "电子测量", "A302", meterId) == OpResult::Ok);
    assert(scopeId == 1);
    assert(meterId == 2);
    assert(manager.deviceCount() == 2);

    // 分类不存在或输入为空时拒绝入库
    int rejectedId = -1;
    assert(manager.addDevice("频谱仪", "不存在的分类", "A303", rejectedId) == OpResult::NotFound);
    assert(manager.addDevice("", "电子测量", "A301", rejectedId) == OpResult::InvalidInput);
    assert(manager.addDevice("频谱仪", "", "A301", rejectedId) == OpResult::InvalidInput);
    assert(manager.addDevice("频谱仪", "电子测量", "", rejectedId) == OpResult::InvalidInput);
    assert(manager.deviceCount() == 2);

    Device device(0, "", "", "");  // 仅作接收容器
    assert(manager.findDevice(scopeId, device));
    assert(device.getName() == "示波器");
    assert(device.getCategory() == "电子测量");
    assert(device.getLocation() == "A301");
    assert(device.getStatus() == DeviceStatus::Idle);

    assert(manager.contains(scopeId));
    assert(!manager.contains(99));
    assert(!manager.findDevice(99, device));
}

void testUpdateDeviceAndStatus() {
    DeviceManager manager;
    int electronics = -1;
    manager.addCategory(-1, "电子测量", electronics);
    int deviceId = 0;
    manager.addDevice("示波器", "电子测量", "A301", deviceId);

    assert(manager.updateDevice(deviceId, "数字示波器", "电子测量", "B502") == OpResult::Ok);

    Device device(0, "", "", "");
    assert(manager.findDevice(deviceId, device));
    assert(device.getName() == "数字示波器");
    assert(device.getLocation() == "B502");

    assert(manager.updateDevice(deviceId, "", "电子测量", "B502") == OpResult::InvalidInput);
    assert(manager.updateDevice(deviceId, "示波器", "不存在", "B502") == OpResult::NotFound);
    assert(manager.updateDevice(99, "示波器", "电子测量", "B502") == OpResult::NotFound);

    assert(manager.setStatus(deviceId, DeviceStatus::Borrowed) == OpResult::Ok);
    assert(manager.findDevice(deviceId, device));
    assert(device.getStatus() == DeviceStatus::Borrowed);
    assert(manager.setStatus(99, DeviceStatus::Idle) == OpResult::NotFound);
}

void testRemoveDevice() {
    DeviceManager manager;
    int electronics = -1;
    manager.addCategory(-1, "电子测量", electronics);
    int first = 0;
    int second = 0;
    manager.addDevice("示波器", "电子测量", "A301", first);
    manager.addDevice("万用表", "电子测量", "A302", second);

    assert(manager.removeDevice(second) == OpResult::Ok);
    assert(manager.removeDevice(second) == OpResult::NotFound);  // 重复删除
    assert(manager.removeDevice(99) == OpResult::NotFound);
    assert(manager.deviceCount() == 1);

    // 借出中的设备不允许删除
    manager.setStatus(first, DeviceStatus::Borrowed);
    assert(manager.removeDevice(first) == OpResult::InvalidState);
    assert(manager.deviceCount() == 1);

    // 编号只增不减
    int third = 0;
    manager.addDevice("频谱仪", "电子测量", "A303", third);
    assert(third == 3);
}

void testRemoveCategory() {
    DeviceManager manager;
    int electronics = -1;
    int scope = -1;
    manager.addCategory(-1, "电子测量", electronics);
    manager.addCategory(electronics, "示波器", scope);
    int deviceId = 0;
    manager.addDevice("示波器一号", "示波器", "A301", deviceId);

    // 分类本身或其子分类被设备使用时拒绝删除
    assert(manager.removeCategory(manager.findCategory("示波器")) == OpResult::InvalidState);
    assert(manager.removeCategory(manager.findCategory("电子测量")) == OpResult::InvalidState);
    assert(manager.categoryCount() == 2);

    // 空分类可以删除
    int spare = -1;
    assert(manager.addCategory(-1, "备用分类", spare) == OpResult::Ok);
    assert(manager.removeCategory(spare) == OpResult::Ok);
    assert(manager.categoryCount() == 2);

    assert(manager.removeCategory(99) == OpResult::NotFound);

    // 设备改到别的分类后原分类可删，删除父分类会连同子树一起消失
    int mechanical = -1;
    assert(manager.addCategory(-1, "机械加工", mechanical) == OpResult::Ok);
    assert(manager.updateDevice(deviceId, "示波器一号", "机械加工", "A301") == OpResult::Ok);
    assert(manager.removeCategory(manager.findCategory("电子测量")) == OpResult::Ok);
    assert(manager.categoryCount() == 1);
    assert(manager.findCategory("示波器") == -1);
    assert(manager.findCategory("机械加工") != -1);
    assert(manager.deviceCount() == 1);
}

void testForEachDeviceAndCategory() {
    DeviceManager manager;
    int electronics = -1;
    int mechanical = -1;
    int scope = -1;
    manager.addCategory(-1, "电子测量", electronics);
    manager.addCategory(-1, "机械加工", mechanical);
    manager.addCategory(electronics, "示波器", scope);

    int deviceId = 0;
    manager.addDevice("示波器一号", "示波器", "A301", deviceId);

    int deviceCount = 0;
    manager.forEachDevice([&deviceCount](const Device &device) {
        ++deviceCount;
        assert(device.getId() > 0);
    });
    assert(deviceCount == 1);

    // 前序：电子测量 -> 示波器 -> 机械加工
    std::string names[3];
    int parents[3];
    int visited = 0;
    manager.forEachCategory([&names, &parents, &visited](const std::string &name, int, int parentId) {
        names[visited] = name;
        parents[visited] = parentId;
        ++visited;
    });
    assert(visited == 3);
    assert(names[0] == "电子测量" && parents[0] == -1);
    assert(names[1] == "示波器" && parents[1] == electronics);
    assert(names[2] == "机械加工" && parents[2] == -1);
}

void testSubmitReservation() {
    ReservationManager manager;
    assert(manager.reservationCount() == 0);

    int first = 0;
    int second = 0;
    assert(manager.submitReservation(1, 101, 600, 720, first) == OpResult::Ok);
    assert(manager.submitReservation(2, 101, 720, 840, second) == OpResult::Ok);
    assert(first == 1);
    assert(second == 2);
    assert(manager.reservationCount() == 2);

    Reservation reservation(0, 0, 0, 0, 0);  // 仅作接收容器
    assert(manager.findReservation(first, reservation));
    assert(reservation.getUserId() == 1);
    assert(reservation.getDeviceId() == 101);
    assert(reservation.getStartTime() == 600);
    assert(reservation.getEndTime() == 720);
    assert(reservation.getStatus() == ReservationStatus::Active);

    assert(manager.contains(first));
    assert(!manager.contains(99));
}

void testReservationTimeValidation() {
    ReservationManager manager;
    int rejected = -1;
    assert(manager.submitReservation(1, 101, 720, 600, rejected) == OpResult::InvalidInput);
    assert(manager.submitReservation(1, 101, 600, 600, rejected) == OpResult::InvalidInput);
    assert(manager.submitReservation(1, 101, -10, 600, rejected) == OpResult::InvalidInput);
    assert(manager.reservationCount() == 0);
}

void testReservationConflict() {
    ReservationManager manager;
    int base = 0;
    assert(manager.submitReservation(1, 101, 600, 720, base) == OpResult::Ok);

    // 部分重叠、完全相同、完全包含、被包含都算冲突
    int rejected = -1;
    assert(manager.submitReservation(2, 101, 660, 780, rejected) == OpResult::TimeConflict);
    assert(manager.submitReservation(2, 101, 600, 720, rejected) == OpResult::TimeConflict);
    assert(manager.submitReservation(2, 101, 580, 740, rejected) == OpResult::TimeConflict);
    assert(manager.submitReservation(2, 101, 620, 700, rejected) == OpResult::TimeConflict);

    // 半开区间 [start, end)：首尾相接不算冲突
    int before = 0;
    int after = 0;
    assert(manager.submitReservation(2, 101, 480, 600, before) == OpResult::Ok);
    assert(manager.submitReservation(2, 101, 720, 840, after) == OpResult::Ok);

    // 不同设备互不影响
    int otherDevice = 0;
    assert(manager.submitReservation(2, 102, 600, 720, otherDevice) == OpResult::Ok);
    assert(manager.reservationCount() == 4);

    assert(manager.hasConflict(101, 610, 620));
    assert(!manager.hasConflict(101, 900, 960));
    assert(manager.hasConflict(101, 700, 600));  // 非法区间直接视为冲突
}

void testCancelReservation() {
    ReservationManager manager;
    int first = 0;
    int second = 0;
    manager.submitReservation(1, 101, 600, 720, first);
    manager.submitReservation(2, 101, 720, 840, second);

    assert(manager.cancelReservation(first) == OpResult::Ok);
    assert(manager.cancelReservation(first) == OpResult::InvalidState);  // 重复取消
    assert(manager.cancelReservation(99) == OpResult::NotFound);

    Reservation reservation(0, 0, 0, 0, 0);
    assert(manager.findReservation(first, reservation));
    assert(reservation.getStatus() == ReservationStatus::Cancelled);

    // 取消后时段释放，可以重新预约
    int again = 0;
    assert(manager.submitReservation(3, 101, 600, 720, again) == OpResult::Ok);
    assert(manager.reservationCount() == 3);
}

void testForEachReservationOfDevice() {
    ReservationManager manager;
    int id = 0;
    manager.submitReservation(1, 101, 600, 720, id);
    manager.submitReservation(2, 101, 720, 840, id);
    manager.submitReservation(3, 102, 600, 720, id);

    int deviceCount = 0;
    manager.forEachReservationOfDevice(101, [&deviceCount](const Reservation &reservation) {
        assert(reservation.getDeviceId() == 101);
        ++deviceCount;
    });
    assert(deviceCount == 2);

    int total = 0;
    manager.forEachReservation([&total](const Reservation &) {
        ++total;
    });
    assert(total == 3);
}

void testWaitingQueue() {
    ReservationManager manager;
    int active = 0;
    manager.submitReservation(1, 101, 600, 720, active);

    int waiting = 0;
    assert(manager.submitWaitingReservation(2, 101, 660, 780, waiting) == OpResult::Ok);
    assert(manager.submitWaitingReservation(3, 101, -10, 780, waiting) == OpResult::InvalidInput);
    assert(manager.waitingCount() == 1);

    Reservation reservation(0, 0, 0, 0, 0);
    assert(manager.findReservation(waiting, reservation));
    assert(reservation.getStatus() == ReservationStatus::Waiting);

    int visited = 0;
    manager.forEachWaiting([&visited, waiting](const Reservation &item) {
        ++visited;
        assert(item.getId() == waiting);
        assert(item.getStatus() == ReservationStatus::Waiting);
    });
    assert(visited == 1);

    // 等待中的预约尚未占用时段，不参与冲突判断
    int others = 0;
    assert(manager.submitReservation(4, 101, 740, 800, others) == OpResult::Ok);
}

void testPromoteWaiting() {
    ReservationManager manager;
    int active = 0;
    int firstWaiting = 0;
    int secondWaiting = 0;
    int otherDevice = 0;
    manager.submitReservation(1, 101, 600, 720, active);
    manager.submitWaitingReservation(2, 101, 620, 700, firstWaiting);
    manager.submitWaitingReservation(3, 101, 640, 720, secondWaiting);
    manager.submitWaitingReservation(4, 102, 600, 720, otherDevice);
    assert(manager.waitingCount() == 3);

    // 时段未释放时提升不了任何任务
    int promotedCount = 0;
    manager.promoteWaitingForDevice(101, [&promotedCount](const Reservation &) {
        ++promotedCount;
    });
    assert(promotedCount == 0);
    assert(manager.waitingCount() == 3);

    // 取消占用时段的预约后，先到先得：队首的能提升，后面的仍与它冲突
    assert(manager.cancelReservation(active) == OpResult::Ok);

    int promoted = 0;
    manager.promoteWaitingForDevice(101, [&promoted, firstWaiting](const Reservation &reservation) {
        ++promoted;
        assert(reservation.getStatus() == ReservationStatus::Active);
        assert(reservation.getId() == firstWaiting);
    });
    assert(promoted == 1);
    assert(manager.waitingCount() == 2);  // 设备 102 的等待任务不受影响

    Reservation reservation(0, 0, 0, 0, 0);
    assert(manager.findReservation(firstWaiting, reservation));
    assert(reservation.getStatus() == ReservationStatus::Active);
    assert(manager.findReservation(secondWaiting, reservation));
    assert(reservation.getStatus() == ReservationStatus::Waiting);

    // 等待任务取消后不再参与调度
    assert(manager.cancelReservation(secondWaiting) == OpResult::Ok);
    assert(manager.waitingCount() == 1);

    promoted = 0;
    manager.promoteWaitingForDevice(101, [&promoted](const Reservation &) {
        ++promoted;
    });
    assert(promoted == 0);

    // 另一个设备的时段释放时才轮到它
    promoted = 0;
    manager.promoteWaitingForDevice(102, [&promoted, otherDevice](const Reservation &reservation) {
        ++promoted;
        assert(reservation.getId() == otherDevice);
    });
    assert(promoted == 1);
    assert(manager.waitingCount() == 0);
}

int main() {
    testAddUser();
    testFindUser();
    testRenameUser();
    testSetDepartment();
    testRemoveUser();
    testForEachUsers();

    testAddCategory();
    testAddDevice();
    testUpdateDeviceAndStatus();
    testRemoveDevice();
    testRemoveCategory();
    testForEachDeviceAndCategory();

    testSubmitReservation();
    testReservationTimeValidation();
    testReservationConflict();
    testCancelReservation();
    testForEachReservationOfDevice();

    testWaitingQueue();
    testPromoteWaiting();

    std::cout << "test_service: all passed" << std::endl;
    return 0;
}
