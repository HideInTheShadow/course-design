#include "gui/ReservationPage.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "gui/GuiUtil.h"
#include "service/LabService.h"

namespace {

QString userName(LabService &service, int userId) {
    User user(0, "", "");  // 仅作接收容器
    if (service.users().findUser(userId, user)) return QString::fromStdString(user.getName());

    return QString("已删除用户 %1").arg(userId);
}

QString deviceName(LabService &service, int deviceId) {
    Device device(0, "", "", "");  // 仅作接收容器
    if (service.devices().findDevice(deviceId, device)) return QString::fromStdString(device.getName());

    return QString("已删除设备 %1").arg(deviceId);
}

QTableWidget *buildTable(QWidget *parent, const QStringList &headers) {
    QTableWidget *table = new QTableWidget(parent);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    return table;
}

}

ReservationPage::ReservationPage(LabService &service, QWidget *parent)
    : QWidget(parent), service(service) {
    reservationTable = buildTable(this, {"编号", "用户", "设备", "开始时间", "结束时间", "状态"});
    waitingTable = buildTable(this, {"排队序号", "预约编号", "用户", "设备", "开始时间", "结束时间"});

    QPushButton *cancelButton = new QPushButton("取消预约", this);
    QPushButton *undoButton = new QPushButton("撤销最近一次取消", this);
    QPushButton *refreshButton = new QPushButton("刷新", this);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(cancelButton);
    buttons->addWidget(undoButton);
    buttons->addWidget(refreshButton);
    buttons->addStretch();

    hintLabel = new QLabel("选中一行后可以取消预约；取消之后可以撤销", this);

    QGroupBox *reservationGroup = new QGroupBox("全部预约", this);
    QVBoxLayout *reservationLayout = new QVBoxLayout(reservationGroup);
    reservationLayout->addWidget(reservationTable);

    QGroupBox *waitingGroup = new QGroupBox("等待任务（按排队顺序）", this);
    QVBoxLayout *waitingLayout = new QVBoxLayout(waitingGroup);
    waitingLayout->addWidget(waitingTable);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(reservationGroup);
    layout->addWidget(waitingGroup);
    layout->addLayout(buttons);
    layout->addWidget(hintLabel);

    connect(cancelButton, &QPushButton::clicked, this, &ReservationPage::onCancelReservation);
    connect(undoButton, &QPushButton::clicked, this, &ReservationPage::onUndoCancel);
    connect(refreshButton, &QPushButton::clicked, this, &ReservationPage::refresh);

    refresh();
}

void ReservationPage::refresh() {
    reloadReservations();
    reloadWaiting();
}

void ReservationPage::reloadReservations() {
    int previousId = selectedReservationId();

    reservationTable->setSortingEnabled(false);
    reservationTable->setRowCount(0);

    service.reservations().forEachReservation([this](const Reservation &reservation) {
        int row = reservationTable->rowCount();
        reservationTable->insertRow(row);
        reservationTable->setItem(row, 0, new QTableWidgetItem(QString::number(reservation.getId())));
        reservationTable->setItem(row, 1, new QTableWidgetItem(userName(service, reservation.getUserId())));
        reservationTable->setItem(row, 2, new QTableWidgetItem(deviceName(service, reservation.getDeviceId())));
        reservationTable->setItem(row, 3, new QTableWidgetItem(formatMinutes(reservation.getStartTime())));
        reservationTable->setItem(row, 4, new QTableWidgetItem(formatMinutes(reservation.getEndTime())));
        reservationTable->setItem(row, 5, new QTableWidgetItem(reservationStatusText(reservation.getStatus())));
    });

    reservationTable->setSortingEnabled(true);
    selectRowById(reservationTable, previousId);
}

void ReservationPage::reloadWaiting() {
    // 队列顺序即等待顺序，不能排序
    waitingTable->setSortingEnabled(false);
    waitingTable->setRowCount(0);

    int queueIndex = 0;
    service.reservations().forEachWaiting([this, &queueIndex](const Reservation &reservation) {
        ++queueIndex;

        int row = waitingTable->rowCount();
        waitingTable->insertRow(row);
        waitingTable->setItem(row, 0, new QTableWidgetItem(QString::number(queueIndex)));
        waitingTable->setItem(row, 1, new QTableWidgetItem(QString::number(reservation.getId())));
        waitingTable->setItem(row, 2, new QTableWidgetItem(userName(service, reservation.getUserId())));
        waitingTable->setItem(row, 3, new QTableWidgetItem(deviceName(service, reservation.getDeviceId())));
        waitingTable->setItem(row, 4, new QTableWidgetItem(formatMinutes(reservation.getStartTime())));
        waitingTable->setItem(row, 5, new QTableWidgetItem(formatMinutes(reservation.getEndTime())));
    });
}

int ReservationPage::selectedReservationId() const {
    int row = reservationTable->currentRow();
    if (row < 0) return -1;

    QTableWidgetItem *item = reservationTable->item(row, 0);
    return item ? item->text().toInt() : -1;
}

void ReservationPage::onCancelReservation() {
    int reservationId = selectedReservationId();
    if (reservationId < 0) {
        hintLabel->setText("请先在列表中选中一条预约");
        return;
    }

    OpResult result = service.cancelReservation(reservationId);
    if (result == OpResult::Ok) {
        hintLabel->setText(QString("预约 %1 已取消，等待中的任务已重新调度").arg(reservationId));
    }
    else {
        hintLabel->setText(QString("取消失败：%1").arg(resultText(result)));
    }
    refresh();
}

void ReservationPage::onUndoCancel() {
    int restoredId = 0;
    OpResult result = service.undoLastCancel(restoredId);

    if (result == OpResult::Ok) {
        hintLabel->setText(QString("已恢复预约 %1").arg(restoredId));
    }
    else if (result == OpResult::NotFound) {
        hintLabel->setText("没有可撤销的操作");
    }
    else {
        hintLabel->setText(QString("撤销失败：%1").arg(resultText(result)));
    }
    refresh();
}
