#pragma once

#include <functional>

#include "datastruct/LinkedList.h"
#include "datastruct/Stack.h"
#include "model/BorrowRecord.h"
#include "service/DeviceManager.h"
#include "service/OpResult.h"
#include "service/ReservationManager.h"
#include "service/UserManager.h"

// 业务协调者：把用户、设备、预约、借用串成完整流程
// 单个对象自身的规则由各 Manager 负责，跨越多个对象的约束集中在这里
class LabService {
private:
    // 可撤销操作的记录：目前只有「取消预约」
    struct UndoAction {
        int reservationId;
        ReservationStatus previousStatus;  // 取消前的状态，撤销时按原状态恢复
    };

    UserManager userManager;
    DeviceManager deviceManager;
    ReservationManager reservationManager;
    LinkedList<BorrowRecord> borrowHistory;  // 借用历史，只追加
    Stack<UndoAction> undoStack;
    int nextBorrowRecordId;

    // 设备状态跟随预约走：有有效预约则「已预约」，没有则「空闲」；借出与维修状态不在此处改动
    void refreshDeviceStatus(int deviceId) {
        Device device(0, "", "", "");  // 仅作接收容器
        if (!deviceManager.findDevice(deviceId, device)) return;
        if (device.getStatus() == DeviceStatus::Borrowed
            || device.getStatus() == DeviceStatus::Repair) {
            return;
        }

        bool hasActive = false;
        reservationManager.forEachReservationOfDevice(deviceId, [&hasActive](const Reservation &reservation) {
            if (reservation.getStatus() == ReservationStatus::Active) hasActive = true;
        });
        deviceManager.setStatus(deviceId, hasActive ? DeviceStatus::Reserved : DeviceStatus::Idle);
    }

public:
    LabService() : nextBorrowRecordId(1) {}

    // 用户与设备自身的增删改查直接转交各自的 Manager
    UserManager &users() {
        return userManager;
    }

    DeviceManager &devices() {
        return deviceManager;
    }

    const UserManager &users() const {
        return userManager;
    }

    const DeviceManager &devices() const {
        return deviceManager;
    }

    // 预约只通过下面的流程方法修改，因此对外只提供只读访问
    const ReservationManager &reservations() const {
        return reservationManager;
    }

    // 载入数据文件：按文件中记录的编号与状态原样恢复，不做日常业务校验
    // 文件由本程序写出，格式错误的记录在读取阶段已被丢弃
    bool restoreReservation(const Reservation &reservation) {
        return reservationManager.restore(reservation);
    }

    void restoreWaitingOrder(int reservationId) {
        reservationManager.enqueueWaiting(reservationId);
    }

    void restoreBorrowRecord(const BorrowRecord &record) {
        if (record.getId() <= 0) return;

        bool duplicated = false;
        borrowHistory.forEach([&duplicated, &record](const BorrowRecord &existing) {
            if (existing.getId() == record.getId()) duplicated = true;
        });
        if (duplicated) return;

        if (record.getId() >= nextBorrowRecordId) nextBorrowRecordId = record.getId() + 1;
        borrowHistory.pushBack(record);
    }

    // 用户还有有效预约或未归还的设备时不允许删除
    OpResult removeUser(int userId) {
        bool busy = false;
        reservationManager.forEachReservation([&busy, userId](const Reservation &reservation) {
            if (busy) return;
            if (reservation.getUserId() != userId) return;
            if (reservation.getStatus() != ReservationStatus::Cancelled) busy = true;
        });
        borrowHistory.forEach([&busy, userId](const BorrowRecord &record) {
            if (record.getUserId() == userId && !record.isReturned()) busy = true;
        });
        if (busy) return OpResult::InvalidState;

        return userManager.removeUser(userId);
    }

    // 设备还有有效预约或等待任务时不允许删除
    OpResult removeDevice(int deviceId) {
        bool busy = false;
        reservationManager.forEachReservationOfDevice(deviceId, [&busy](const Reservation &reservation) {
            if (busy) return;
            if (reservation.getStatus() != ReservationStatus::Cancelled) busy = true;
        });
        if (busy) return OpResult::InvalidState;

        return deviceManager.removeDevice(deviceId);
    }

    // 提交预约：用户与设备必须存在，时间冲突时返回 TimeConflict，由调用方决定是否排队
    OpResult reserve(int userId, int deviceId, long long startTime, long long endTime,
                     int &reservationId) {
        if (!userManager.contains(userId)) return OpResult::NotFound;
        if (!deviceManager.contains(deviceId)) return OpResult::NotFound;

        OpResult result = reservationManager.submitReservation(userId, deviceId, startTime,
                                                               endTime, reservationId);
        if (result == OpResult::Ok) refreshDeviceStatus(deviceId);
        return result;
    }

    // 冲突时选择排队等待；若此刻已不冲突（例如对方刚取消），则直接按有效预约建立
    OpResult waitReservation(int userId, int deviceId, long long startTime, long long endTime,
                             int &reservationId) {
        if (!userManager.contains(userId)) return OpResult::NotFound;
        if (!deviceManager.contains(deviceId)) return OpResult::NotFound;

        OpResult result = OpResult::Ok;
        if (reservationManager.hasConflict(deviceId, startTime, endTime)) {
            result = reservationManager.submitWaitingReservation(userId, deviceId, startTime,
                                                                 endTime, reservationId);
        }
        else {
            result = reservationManager.submitReservation(userId, deviceId, startTime, endTime,
                                                          reservationId);
        }

        if (result == OpResult::Ok) refreshDeviceStatus(deviceId);
        return result;
    }

    // 取消预约并记录撤销信息；有效预约被取消会释放时段，等待任务随之前移
    OpResult cancelReservation(int reservationId) {
        Reservation reservation(0, 0, 0, 0, 0);  // 仅作接收容器
        if (!reservationManager.findReservation(reservationId, reservation)) return OpResult::NotFound;

        ReservationStatus previousStatus = reservation.getStatus();
        OpResult result = reservationManager.cancelReservation(reservationId);
        if (result != OpResult::Ok) return result;

        if (previousStatus == ReservationStatus::Active) {
            reservationManager.promoteWaitingForDevice(reservation.getDeviceId());
        }
        refreshDeviceStatus(reservation.getDeviceId());

        undoStack.push(UndoAction{reservationId, previousStatus});
        return OpResult::Ok;
    }

    // 撤销最近一次取消；时段已被别人占用时返回 TimeConflict，操作记录保留以便稍后再试
    OpResult undoLastCancel(int &restoredId) {
        UndoAction action{0, ReservationStatus::Active};
        if (!undoStack.top(action)) return OpResult::NotFound;

        Reservation reservation(0, 0, 0, 0, 0);  // 仅作接收容器
        if (!reservationManager.findReservation(action.reservationId, reservation)) {
            return OpResult::NotFound;
        }

        OpResult result = reservationManager.restoreReservation(action.reservationId,
                                                                action.previousStatus);
        if (result != OpResult::Ok) return result;

        undoStack.pop();
        refreshDeviceStatus(reservation.getDeviceId());
        restoredId = action.reservationId;
        return OpResult::Ok;
    }

    int undoDepth() const {
        return undoStack.size();
    }

    // 借用：设备可用、该用户有一条覆盖当前时刻的有效预约、且当前没有未归还的借用记录
    OpResult borrowDevice(int deviceId, int userId, long long borrowTime, int &recordId) {
        if (borrowTime < 0) return OpResult::InvalidInput;
        if (!userManager.contains(userId)) return OpResult::NotFound;

        Device device(0, "", "", "");  // 仅作接收容器
        if (!deviceManager.findDevice(deviceId, device)) return OpResult::NotFound;
        if (device.getStatus() == DeviceStatus::Borrowed) return OpResult::InvalidState;
        if (device.getStatus() == DeviceStatus::Repair) return OpResult::InvalidState;

        BorrowRecord open(0, 0, 0, 0);  // 仅作接收容器
        if (findOpenBorrow(deviceId, open)) return OpResult::InvalidState;

        bool covered = false;
        reservationManager.forEachReservationOfDevice(
            deviceId, [&covered, userId, borrowTime](const Reservation &reservation) {
                if (covered) return;
                if (reservation.getUserId() != userId) return;
                if (reservation.getStatus() != ReservationStatus::Active) return;
                if (reservation.getStartTime() <= borrowTime
                    && borrowTime < reservation.getEndTime()) {
                    covered = true;
                }
            });
        if (!covered) return OpResult::InvalidState;

        recordId = nextBorrowRecordId++;
        borrowHistory.pushBack(BorrowRecord(recordId, deviceId, userId, borrowTime));
        deviceManager.setStatus(deviceId, DeviceStatus::Borrowed);
        return OpResult::Ok;
    }

    // 归还：登记归还时间，设备状态按是否还有有效预约重新计算，等待任务随之前移
    OpResult returnDevice(int deviceId, long long returnTime) {
        if (returnTime < 0) return OpResult::InvalidInput;

        Device device(0, "", "", "");  // 仅作接收容器
        if (!deviceManager.findDevice(deviceId, device)) return OpResult::NotFound;
        if (device.getStatus() != DeviceStatus::Borrowed) return OpResult::InvalidState;

        bool marked = false;
        borrowHistory.forEachMutable([&marked, deviceId, returnTime](BorrowRecord &record) {
            if (marked) return;
            if (record.getDeviceId() != deviceId || record.isReturned()) return;

            record.markReturned(returnTime);
            marked = true;
        });
        if (!marked) return OpResult::InvalidState;  // 设备状态与借用历史不一致

        // 先解除借出状态，再按剩余的有效预约重新计算
        deviceManager.setStatus(deviceId, DeviceStatus::Idle);
        reservationManager.promoteWaitingForDevice(deviceId);
        refreshDeviceStatus(deviceId);
        return OpResult::Ok;
    }

    bool findOpenBorrow(int deviceId, BorrowRecord &out) const {
        bool found = false;
        borrowHistory.forEach([&found, &out, deviceId](const BorrowRecord &record) {
            if (found) return;
            if (record.getDeviceId() != deviceId || record.isReturned()) return;

            out = record;
            found = true;
        });
        return found;
    }

    void forEachBorrowRecord(const std::function<void(const BorrowRecord &)> &fn) const {
        borrowHistory.forEach(fn);
    }

    int borrowRecordCount() const {
        return borrowHistory.size();
    }
};
