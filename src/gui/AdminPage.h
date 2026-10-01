#ifndef ADMINPAGE_H
#define ADMINPAGE_H

#include <QWidget>

class QLabel;
class QTableWidget;
class QTreeWidget;

class LabService;

// 管理页：用户管理、设备分类管理、设备管理
class AdminPage : public QWidget {
    Q_OBJECT

public:
    explicit AdminPage(LabService &service, QWidget *parent = nullptr);

    void refresh();

private:
    QWidget *buildUserPanel();
    QWidget *buildCategoryPanel();
    QWidget *buildDevicePanel();

    int selectedUserId() const;
    int selectedDeviceId() const;
    int selectedCategoryId() const;

    void reloadUsers();
    void reloadCategories();
    void reloadDevices();

    void onAddUser();
    void onRenameUser();
    void onSetDepartment();
    void onRemoveUser();

    void onAddRootCategory();
    void onAddChildCategory();
    void onRemoveCategory();

    void onAddDevice();
    void onUpdateDevice();
    void onRemoveDevice();
    void onMarkRepair();
    void onClearRepair();

    LabService &service;

    QTableWidget *userTable;
    QTableWidget *deviceTable;
    QTreeWidget *categoryTree;
    QLabel *userHint;
    QLabel *categoryHint;
    QLabel *deviceHint;
};

#endif
