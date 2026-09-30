#pragma once

enum class BorrowStatus {
    Borrowed,  // 借出中
    Returned   // 已归还
};

class BorrowRecord {
private:
    int id;
    int deviceId;
    int userId;
    long long borrowTime;
    long long returnTime;  // 0 表示尚未归还
    BorrowStatus status;

public:
    BorrowRecord(int id, int deviceId, int userId, long long borrowTime)
        : id(id), deviceId(deviceId), userId(userId),
          borrowTime(borrowTime), returnTime(0),
          status(BorrowStatus::Borrowed) {}

    int getId() const {
        return id;
    }

    int getDeviceId() const {
        return deviceId;
    }

    int getUserId() const {
        return userId;
    }

    long long getBorrowTime() const {
        return borrowTime;
    }

    long long getReturnTime() const {
        return returnTime;
    }

    BorrowStatus getStatus() const {
        return status;
    }

    bool isReturned() const {
        return status == BorrowStatus::Returned;
    }

    void markReturned(long long returnTime) {
        this->returnTime = returnTime;
        status = BorrowStatus::Returned;
    }
};
