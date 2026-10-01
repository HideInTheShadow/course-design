#pragma once

#include <functional>

#include "datastruct/HashTable.h"
#include "datastruct/Queue.h"
#include "model/Reservation.h"
#include "service/OpResult.h"

// 预约管理：编号到 Reservation 对象的哈希表，另用队列保存等待任务的先后次序
// 等待中的预约实体同样存放在哈希表里，队列只存编号，避免同一份数据两处保存
// 用户与设备的合法存在性由上层协调者校验，这里只管预约自身的规则：时间合法性与同设备时间冲突
class ReservationManager {
private:
    HashTable<int, Reservation> reservations;
    Queue<int> waitingOrder;  // 等待任务的预约编号，先进先出
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

    // 时间冲突时用户可以选择排队等待：建立等待中的预约并排到队尾
    // 队列只在设备时段释放后由 promoteWaitingForDevice 处理
    OpResult submitWaitingReservation(int userId, int deviceId, long long startTime,
                                      long long endTime, int &newId) {
        if (startTime < 0 || startTime >= endTime) return OpResult::InvalidInput;

        newId = nextId++;
        reservations.insert(newId, Reservation(newId, userId, deviceId, startTime, endTime,
                                               ReservationStatus::Waiting));
        waitingOrder.push(newId);
        return OpResult::Ok;
    }

    // 设备的时段释放后按等待顺序处理：不再冲突的等待任务提升为有效预约，返回提升数量
    int promoteWaitingForDevice(int deviceId) {
        // 只处理当前队列中的任务，本轮新排回队尾的不再重复检查
        int pending = waitingOrder.size();
        int promotedCount = 0;

        for (int i = 0; i < pending; ++i) {
            int reservationId = 0;
            waitingOrder.front(reservationId);
            waitingOrder.pop();

            Reservation reservation(0, 0, 0, 0, 0);  // 仅作接收容器
            if (!reservations.find(reservationId, reservation)) continue;
            if (reservation.getStatus() != ReservationStatus::Waiting) continue;  // 已被取消

            bool promoted = false;
            if (reservation.getDeviceId() == deviceId
                && !conflictsWithExisting(deviceId, reservation.getStartTime(),
                                          reservation.getEndTime())) {
                reservation.setStatus(ReservationStatus::Active);
                reservations.update(reservationId, reservation);
                ++promotedCount;
                promoted = true;
            }

            // 提升成功的不再排队，其余保持原有先后次序
            if (!promoted) waitingOrder.push(reservationId);
        }
        return promotedCount;
    }

    // 撤销取消：按取消前的状态恢复预约；恢复有效预约时若时段已被占用则拒绝
    OpResult restoreReservation(int reservationId, ReservationStatus previousStatus) {
        Reservation reservation(0, 0, 0, 0, 0);  // 仅作接收容器
        if (!reservations.find(reservationId, reservation)) return OpResult::NotFound;
        if (reservation.getStatus() != ReservationStatus::Cancelled) return OpResult::InvalidState;
        if (previousStatus == ReservationStatus::Active
            && conflictsWithExisting(reservation.getDeviceId(), reservation.getStartTime(),
                                     reservation.getEndTime())) {
            return OpResult::TimeConflict;
        }

        reservation.setStatus(previousStatus);
        reservations.update(reservationId, reservation);

        // 恢复的等待任务重新排到队尾，不打乱队列中其它任务的先后次序
        if (previousStatus == ReservationStatus::Waiting) waitingOrder.push(reservationId);
        return OpResult::Ok;
    }

    int waitingCount() const {
        int count = 0;
        reservations.forEach([&count](const int &, const Reservation &reservation) {
            if (reservation.getStatus() == ReservationStatus::Waiting) ++count;
        });
        return count;
    }

    // 按排队先后输出等待任务；已被取消的预约跳过
    void forEachWaiting(const std::function<void(const Reservation &)> &fn) const {
        waitingOrder.forEach([this, &fn](const int &reservationId) {
            Reservation reservation(0, 0, 0, 0, 0);  // 仅作接收容器
            if (!reservations.find(reservationId, reservation)) return;
            if (reservation.getStatus() != ReservationStatus::Waiting) return;

            fn(reservation);
        });
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

    // 载入数据文件：按文件中记录的编号与状态原样写回，等待任务的排队次序随后由 enqueueWaiting 恢复
    bool restore(const Reservation &reservation) {
        if (reservation.getId() <= 0) return false;
        if (!reservation.isValidTimeRange()) return false;
        if (reservations.contains(reservation.getId())) return false;

        reservations.insert(reservation.getId(), reservation);
        if (reservation.getId() >= nextId) nextId = reservation.getId() + 1;
        return true;
    }

    // 载入数据文件：按文件中的先后次序把等待任务重新排入队列
    void enqueueWaiting(int reservationId) {
        Reservation reservation(0, 0, 0, 0, 0);  // 仅作接收容器
        if (!reservations.find(reservationId, reservation)) return;
        if (reservation.getStatus() != ReservationStatus::Waiting) return;

        waitingOrder.push(reservationId);
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
