#pragma once

#include <QDateTime>
#include <QString>
#include <QTableWidget>

#include "model/BorrowRecord.h"
#include "model/Device.h"
#include "model/Reservation.h"
#include "service/OpResult.h"

// 界面上的时间显示与业务层的分钟数互相转换
inline long long toMinutes(const QDateTime &dateTime) {
    return dateTime.toSecsSinceEpoch() / 60;
}

inline QString formatMinutes(long long minutes) {
    return QDateTime::fromSecsSinceEpoch(minutes * 60).toString("yyyy-MM-dd HH:mm");
}

// 操作结果、状态的文字说明集中在这里，业务层只返回枚举
inline QString resultText(OpResult result) {
    switch (result) {
    case OpResult::Ok:
        return "操作成功";
    case OpResult::InvalidInput:
        return "输入不能为空，且开始时间必须早于结束时间";
    case OpResult::NotFound:
        return "对象不存在";
    case OpResult::Duplicate:
        return "同一层级下名称重复";
    case OpResult::TimeConflict:
        return "时间与已有预约冲突";
    case OpResult::InvalidState:
        return "当前状态不允许该操作";
    }
    return "未知结果";
}

inline QString deviceStatusText(DeviceStatus status) {
    switch (status) {
    case DeviceStatus::Idle:
        return "空闲";
    case DeviceStatus::Reserved:
        return "已预约";
    case DeviceStatus::Borrowed:
        return "已借出";
    case DeviceStatus::Repair:
        return "维修中";
    }
    return "未知状态";
}

inline QString reservationStatusText(ReservationStatus status) {
    switch (status) {
    case ReservationStatus::Active:
        return "有效";
    case ReservationStatus::Cancelled:
        return "已取消";
    case ReservationStatus::Waiting:
        return "等待中";
    }
    return "未知状态";
}

inline QString borrowStatusText(BorrowStatus status) {
    switch (status) {
    case BorrowStatus::Borrowed:
        return "借出中";
    case BorrowStatus::Returned:
        return "已归还";
    }
    return "未知状态";
}

// 重填表格后按第一列的编号恢复选中行
inline void selectRowById(QTableWidget *table, int id) {
    if (id < 0) return;

    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem *item = table->item(row, 0);
        if (item && item->text().toInt() == id) {
            table->selectRow(row);
            return;
        }
    }
}
