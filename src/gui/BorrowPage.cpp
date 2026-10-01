#include "gui/BorrowPage.h"

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

}

BorrowPage::BorrowPage(LabService &service, QWidget *parent)
    : QWidget(parent), service(service) {
    historyTable = new QTableWidget(this);
    historyTable->setColumnCount(6);
    historyTable->setHorizontalHeaderLabels({"记录编号", "设备", "用户", "借用时间", "归还时间", "状态"});
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->verticalHeader()->setVisible(false);
    historyTable->horizontalHeader()->setStretchLastSection(true);

    QPushButton *refreshButton = new QPushButton("刷新", this);
    QLabel *hintLabel = new QLabel("借用历史按借用先后排列，已归还的记录保留备查", this);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(refreshButton);
    buttons->addStretch();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(historyTable);
    layout->addLayout(buttons);
    layout->addWidget(hintLabel);

    connect(refreshButton, &QPushButton::clicked, this, &BorrowPage::refresh);

    refresh();
}

void BorrowPage::refresh() {
    historyTable->setSortingEnabled(false);
    historyTable->setRowCount(0);

    service.forEachBorrowRecord([this](const BorrowRecord &record) {
        int row = historyTable->rowCount();
        historyTable->insertRow(row);
        historyTable->setItem(row, 0, new QTableWidgetItem(QString::number(record.getId())));
        historyTable->setItem(row, 1, new QTableWidgetItem(deviceName(service, record.getDeviceId())));
        historyTable->setItem(row, 2, new QTableWidgetItem(userName(service, record.getUserId())));
        historyTable->setItem(row, 3, new QTableWidgetItem(formatMinutes(record.getBorrowTime())));
        historyTable->setItem(row, 4, new QTableWidgetItem(
            record.isReturned() ? formatMinutes(record.getReturnTime()) : QString("-")));
        historyTable->setItem(row, 5, new QTableWidgetItem(borrowStatusText(record.getStatus())));
    });
}
