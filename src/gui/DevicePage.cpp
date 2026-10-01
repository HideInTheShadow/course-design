#include "gui/DevicePage.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "gui/GuiUtil.h"
#include "service/LabService.h"

namespace {

const int CategoryIdRole = Qt::UserRole;

}

DevicePage::DevicePage(LabService &service, QWidget *parent)
    : QWidget(parent), service(service) {
    categoryTree = new QTreeWidget(this);
    categoryTree->setHeaderLabel("设备分类");
    categoryTree->setMinimumWidth(220);

    deviceTable = new QTableWidget(this);
    deviceTable->setColumnCount(5);
    deviceTable->setHorizontalHeaderLabels({"编号", "名称", "分类", "位置", "状态"});
    deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceTable->verticalHeader()->setVisible(false);
    deviceTable->horizontalHeader()->setStretchLastSection(true);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(categoryTree);
    splitter->addWidget(deviceTable);
    splitter->setStretchFactor(1, 1);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
    layout->addWidget(buildOperationPanel());

    connect(categoryTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        reloadDevices();
    });

    refresh();
}

QWidget *DevicePage::buildOperationPanel() {
    QWidget *panel = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(panel);

    userBox = new QComboBox(panel);
    userBox->setMinimumWidth(180);

    startEdit = new QDateTimeEdit(QDateTime::currentDateTime(), panel);
    endEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(3600), panel);
    for (QDateTimeEdit *edit : {startEdit, endEdit}) {
        edit->setDisplayFormat("yyyy-MM-dd HH:mm");
        edit->setCalendarPopup(true);
    }

    QPushButton *reserveButton = new QPushButton("提交预约", panel);
    QPushButton *waitButton = new QPushButton("排队等待", panel);
    QPushButton *borrowButton = new QPushButton("借用", panel);
    QPushButton *returnButton = new QPushButton("归还", panel);
    QPushButton *refreshButton = new QPushButton("刷新", panel);

    QHBoxLayout *controls = new QHBoxLayout();
    controls->addWidget(new QLabel("用户:", panel));
    controls->addWidget(userBox);
    controls->addWidget(new QLabel("开始:", panel));
    controls->addWidget(startEdit);
    controls->addWidget(new QLabel("结束:", panel));
    controls->addWidget(endEdit);
    controls->addWidget(reserveButton);
    controls->addWidget(waitButton);
    controls->addWidget(borrowButton);
    controls->addWidget(returnButton);
    controls->addWidget(refreshButton);
    controls->addStretch();

    hintLabel = new QLabel("选中设备后可以提交预约或借用", panel);

    layout->addLayout(controls);
    layout->addWidget(hintLabel);

    connect(reserveButton, &QPushButton::clicked, this, &DevicePage::onSubmitReservation);
    connect(waitButton, &QPushButton::clicked, this, &DevicePage::onWaitReservation);
    connect(borrowButton, &QPushButton::clicked, this, &DevicePage::onBorrow);
    connect(returnButton, &QPushButton::clicked, this, &DevicePage::onReturn);
    connect(refreshButton, &QPushButton::clicked, this, &DevicePage::refresh);
    return panel;
}

void DevicePage::refresh() {
    reloadCategories();
    reloadUsers();
    reloadDevices();
}

void DevicePage::reloadUsers() {
    int previousId = userBox->currentData().toInt();

    userBox->clear();
    service.users().forEach([this](const User &user) {
        QString text = QString("%1（%2）").arg(QString::fromStdString(user.getName()),
                                            QString::fromStdString(user.getDepartment()));
        userBox->addItem(text, user.getId());
    });

    int index = userBox->findData(previousId);
    if (index >= 0) userBox->setCurrentIndex(index);
}

void DevicePage::reloadCategories() {
    int previousNode = -1;
    QTreeWidgetItem *current = categoryTree->currentItem();
    if (current) previousNode = current->data(0, CategoryIdRole).toInt();

    categoryTree->clear();

    QTreeWidgetItem *root = new QTreeWidgetItem(categoryTree);
    root->setText(0, "全部设备");
    root->setData(0, CategoryIdRole, -1);

    // 前序遍历保证父分类先于子分类出现，可以直接按父下标找控件项
    QHash<int, QTreeWidgetItem *> items;
    items.insert(-1, root);
    service.devices().forEachCategory([&items](const std::string &name, int nodeId, int parentId) {
        QTreeWidgetItem *parent = items.value(parentId, nullptr);
        if (!parent) return;

        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        item->setText(0, QString::fromStdString(name));
        item->setData(0, CategoryIdRole, nodeId);
        items.insert(nodeId, item);
    });
    categoryTree->expandAll();

    QTreeWidgetItem *restored = items.value(previousNode, root);
    categoryTree->setCurrentItem(restored);
}

void DevicePage::collectCategoryNames(QTreeWidgetItem *item, QStringList &names) const {
    int nodeId = item->data(0, CategoryIdRole).toInt();
    if (nodeId >= 0) names.append(item->text(0));

    for (int i = 0; i < item->childCount(); ++i) {
        collectCategoryNames(item->child(i), names);
    }
}

void DevicePage::reloadDevices() {
    int previousDeviceId = selectedDeviceId();

    QStringList categoryNames;
    QTreeWidgetItem *current = categoryTree->currentItem();
    if (current) collectCategoryNames(current, categoryNames);

    deviceTable->setRowCount(0);
    int restoreRow = -1;

    service.devices().forEachDevice([this, &categoryNames, previousDeviceId, &restoreRow](const Device &device) {
        QString category = QString::fromStdString(device.getCategory());
        if (!categoryNames.isEmpty() && !categoryNames.contains(category)) return;

        int row = deviceTable->rowCount();
        deviceTable->insertRow(row);
        deviceTable->setItem(row, 0, new QTableWidgetItem(QString::number(device.getId())));
        deviceTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(device.getName())));
        deviceTable->setItem(row, 2, new QTableWidgetItem(category));
        deviceTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(device.getLocation())));
        deviceTable->setItem(row, 4, new QTableWidgetItem(deviceStatusText(device.getStatus())));

        if (device.getId() == previousDeviceId) restoreRow = row;
    });

    if (restoreRow >= 0) {
        deviceTable->selectRow(restoreRow);
    }
}

int DevicePage::selectedDeviceId() const {
    int row = deviceTable->currentRow();
    if (row < 0) return -1;

    QTableWidgetItem *item = deviceTable->item(row, 0);
    return item ? item->text().toInt() : -1;
}

void DevicePage::onSubmitReservation() {
    if (userBox->currentIndex() < 0) {
        hintLabel->setText("请先在管理页添加用户");
        return;
    }
    int deviceId = selectedDeviceId();
    if (deviceId < 0) {
        hintLabel->setText("请先在列表中选择设备");
        return;
    }

    int userId = userBox->currentData().toInt();
    int reservationId = 0;
    OpResult result = service.reserve(userId, deviceId, toMinutes(startEdit->dateTime()),
                                      toMinutes(endEdit->dateTime()), reservationId);

    if (result == OpResult::Ok) {
        hintLabel->setText(QString("预约成功，预约编号 %1").arg(reservationId));
    }
    else {
        hintLabel->setText(QString("预约失败：%1").arg(resultText(result)));
    }
    refresh();
}

void DevicePage::onWaitReservation() {
    if (userBox->currentIndex() < 0) {
        hintLabel->setText("请先在管理页添加用户");
        return;
    }
    int deviceId = selectedDeviceId();
    if (deviceId < 0) {
        hintLabel->setText("请先在列表中选择设备");
        return;
    }

    int userId = userBox->currentData().toInt();
    int reservationId = 0;
    OpResult result = service.waitReservation(userId, deviceId, toMinutes(startEdit->dateTime()),
                                              toMinutes(endEdit->dateTime()), reservationId);

    if (result == OpResult::Ok) {
        hintLabel->setText(QString("已登记，预约编号 %1（若无冲突则直接生效）").arg(reservationId));
    }
    else {
        hintLabel->setText(QString("登记失败：%1").arg(resultText(result)));
    }
    refresh();
}

void DevicePage::onBorrow() {
    if (userBox->currentIndex() < 0) {
        hintLabel->setText("请先在管理页添加用户");
        return;
    }
    int deviceId = selectedDeviceId();
    if (deviceId < 0) {
        hintLabel->setText("请先在列表中选择设备");
        return;
    }

    int userId = userBox->currentData().toInt();
    int recordId = 0;
    OpResult result = service.borrowDevice(deviceId, userId, toMinutes(QDateTime::currentDateTime()),
                                           recordId);

    if (result == OpResult::Ok) {
        hintLabel->setText(QString("借用成功，记录编号 %1").arg(recordId));
    }
    else {
        hintLabel->setText(QString("借用失败：%1（需要一条覆盖当前时刻的有效预约）")
                               .arg(resultText(result)));
    }
    refresh();
}

void DevicePage::onReturn() {
    int deviceId = selectedDeviceId();
    if (deviceId < 0) {
        hintLabel->setText("请先在列表中选择设备");
        return;
    }

    OpResult result = service.returnDevice(deviceId, toMinutes(QDateTime::currentDateTime()));
    if (result == OpResult::Ok) {
        hintLabel->setText("归还成功");
    }
    else {
        hintLabel->setText(QString("归还失败：%1").arg(resultText(result)));
    }
    refresh();
}
