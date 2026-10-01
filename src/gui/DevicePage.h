#ifndef DEVICEPAGE_H
#define DEVICEPAGE_H

#include <QWidget>

#include "service/OpResult.h"

class QComboBox;
class QDateTimeEdit;
class QLabel;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;

class LabService;

// 设备浏览页：左侧分类树，右侧设备列表，下方是预约、借用、归还操作
class DevicePage : public QWidget {
    Q_OBJECT

public:
    explicit DevicePage(LabService &service, QWidget *parent = nullptr);

    void refresh();

private:
    QWidget *buildOperationPanel();

    int selectedDeviceId() const;
    void collectCategoryNames(QTreeWidgetItem *item, QStringList &names) const;

    void reloadCategories();
    void reloadUsers();
    void reloadDevices();

    void onSubmitReservation();
    void onWaitReservation();
    void onBorrow();
    void onReturn();

    LabService &service;

    QTreeWidget *categoryTree;
    QTableWidget *deviceTable;
    QComboBox *userBox;
    QDateTimeEdit *startEdit;
    QDateTimeEdit *endEdit;
    QLabel *hintLabel;
};

#endif
