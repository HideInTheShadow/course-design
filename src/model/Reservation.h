#pragma once

enum class ReservationStatus {
    Active,     // 有效
    Cancelled,  // 已取消
    Waiting     // 等待中
};

class Reservation {
private:
    int id;
    int userId;
    int deviceId;
    long long startTime;  // 分钟粒度
    long long endTime;
    ReservationStatus status;

public:
    Reservation(int id, int userId, int deviceId,
                long long startTime, long long endTime,
                ReservationStatus status = ReservationStatus::Active)
        : id(id), userId(userId), deviceId(deviceId),
          startTime(startTime), endTime(endTime), status(status) {}

    int getId() const {
        return id;
    }

    int getUserId() const {
        return userId;
    }

    int getDeviceId() const {
        return deviceId;
    }

    long long getStartTime() const {
        return startTime;
    }

    long long getEndTime() const {
        return endTime;
    }

    ReservationStatus getStatus() const {
        return status;
    }

    void setStartTime(long long startTime) {
        this->startTime = startTime;
    }

    void setEndTime(long long endTime) {
        this->endTime = endTime;
    }

    void setStatus(ReservationStatus status) {
        this->status = status;
    }

    bool isValidTimeRange() const {
        return startTime < endTime;
    }

    // 使用半开区间 [start, end)，相邻区间的首尾相接不算冲突
    bool conflictsWith(const Reservation &other) const {
        return startTime < other.endTime && other.startTime < endTime;
    }
};
