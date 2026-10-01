#pragma once

#include <functional>

#include "datastruct/HashTable.h"
#include "model/Reservation.h"
#include "service/OpResult.h"

// 预约管理：编号到 Reservation 对象的哈希表
// 用户与设备的合法存在性由上层协调者校验，这里只管预约自身的规则：时间合法性与同设备时间冲突
class ReservationManager {
private:
    HashTable<int, Reservation> reservations;
    int nextId;

    // 同一设备上是否存在与 [startTime, endTime) 冲突的有效预约
    // 已取消的预约不占用时间，等待中的预约尚未获得时段，都不参与冲突判断
    bool conflictsWithExisting(int deviceId, long long startTime, long long endTime) const {
        Reservation candidate(0, 0, deviceId, startTime, endTime);
        bool conflict = false;

        reservations.forEach([&conflict, &candidate](const int &, const Reservation &reservation) {
            if (conflict) return;
            if (reservation.getDeviceId() != candidate.getDeviceId()) return;
            if (reservation.getStatus() != ReservationStatus::Active) return;
            if (candidate.conflictsWith(reservation)) conflict = true;
        });
        return conflict;
    }

public:
    ReservationManager() : nextId(1) {}

    // 提交预约，成功时通过 newId 返回分配的编号
    OpResult submitReservation(int userId, int deviceId, long long startTime, long long endTime,
                               int &newId) {
        if (startTime < 0 || startTime >= endTime) return OpResult::InvalidInput;
        if (conflictsWithExisting(deviceId, startTime, endTime)) return OpResult::TimeConflict;

        newId = nextId++;
        reservations.insert(newId, Reservation(newId, userId, deviceId, startTime, endTime));
        return OpResult::Ok;
    }

    // 有效预约与等待中的预约都可以取消，重复取消返回 InvalidState
    OpResult cancelReservation(int reservationId) {
        Reservation reservation(0, 0, 0, 0, 0);  // 仅作接收容器
        if (!reservations.find(reservationId, reservation)) return OpResult::NotFound;
        if (reservation.getStatus() == ReservationStatus::Cancelled) return OpResult::InvalidState;

        reservation.setStatus(ReservationStatus::Cancelled);
        reservations.update(reservationId, reservation);
        return OpResult::Ok;
    }

    bool hasConflict(int deviceId, long long startTime, long long endTime) const {
        if (startTime < 0 || startTime >= endTime) return true;

        return conflictsWithExisting(deviceId, startTime, endTime);
    }

    bool findReservation(int reservationId, Reservation &out) const {
        return reservations.find(reservationId, out);
    }

    bool contains(int reservationId) const {
        return reservations.contains(reservationId);
    }

    int reservationCount() const {
        return reservations.size();
    }

    void forEachReservation(const std::function<void(const Reservation &)> &fn) const {
        reservations.forEach([&fn](const int &, const Reservation &reservation) {
            fn(reservation);
        });
    }

    // 供设备详情、等待任务调度使用
    void forEachReservationOfDevice(int deviceId,
                                    const std::function<void(const Reservation &)> &fn) const {
        reservations.forEach([&fn, deviceId](const int &, const Reservation &reservation) {
            if (reservation.getDeviceId() == deviceId) fn(reservation);
        });
    }
};
