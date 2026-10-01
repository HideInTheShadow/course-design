#include "gui/AdminPage.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "gui/GuiUtil.h"
#include "service/LabService.h"

namespace {

const int CategoryIdRole = Qt::UserRole;

// 新增与修改设备共用的小对话框；不需要自定义信号，因此不必使用 Q_OBJECT
class DeviceDialog : public QDialog {
public:
    explicit DeviceDialog(QWidget *parent)
        : QDialog(parent) {
        setWindowTitle("设备信息");
        setMinimumWidth(320);

        nameEdit = new QLineEdit(this);
        categoryBox = new QComboBox(this);
        locationEdit = new QLineEdit(this);

        QFormLayout *form = new QFormLayout();
        form->addRow("设备名称:", nameEdit);
        form->addRow("所属分类:", categoryBox);
        form->addRow("存放位置:", locationEdit);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                                         | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addLayout(form);
        layout->addWidget(buttons);
    }

    void setCategories(const QStringList &names, const QString &current) {
        categoryBox->addItems(names);
        int index = categoryBox->findText(current);
        if (index >= 0) categoryBox->setCurrentIndex(index);
    }

    void setDevice(const QString &name, const QString &location) {
        nameEdit->setText(name);
        locationEdit->setText(location);
    }

    QString name() const {
        return nameEdit->text().trimmed();
    }

    QString category() const {
        return categoryBox->currentText();
    }

    QString location() const {
        return locationEdit->text().trimmed();
    }

private:
    QLineEdit *nameEdit;
    QComboBox *categoryBox;
    QLineEdit *locationEdit;
};

QStringList allCategoryNames(LabService &service) {
    QStringList names;
    service.devices().forEachCategory([&names](const std::string &name, int, int) {
        names.append(QString::fromStdString(name));
    });
    return names;
}

}

AdminPage::AdminPage(LabService &service, QWidget *parent)
    : QWidget(parent), service(service) {
    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(buildUserPanel(), "用户管理");
    tabs->addTab(buildCategoryPanel(), "分类管理");
    tabs->addTab(buildDevicePanel(), "设备管理");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);

    refresh();
}

QWidget *AdminPage::buildUserPanel() {
    QWidget *panel = new QWidget(this);

    userTable = new QTableWidget(panel);
    userTable->setColumnCount(3);
    userTable->setHorizontalHeaderLabels({"编号", "姓名", "部门"});
    userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    userTable->setSelectionMode(QAbstractItemView::SingleSelection);
    userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    userTable->verticalHeader()->setVisible(false);
    userTable->horizontalHeader()->setStretchLastSection(true);

    QPushButton *addButton = new QPushButton("新增用户", panel);
    QPushButton *renameButton = new QPushButton("修改姓名", panel);
    QPushButton *departmentButton = new QPushButton("修改部门", panel);
    QPushButton *removeButton = new QPushButton("删除用户", panel);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(addButton);
    buttons->addWidget(renameButton);
    buttons->addWidget(departmentButton);
    buttons->addWidget(removeButton);
    buttons->addStretch();

    userHint = new QLabel("还有未取消预约或未归还设备的用户不能删除", panel);

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->addWidget(userTable);
    layout->addLayout(buttons);
    layout->addWidget(userHint);

    connect(addButton, &QPushButton::clicked, this, &AdminPage::onAddUser);
    connect(renameButton, &QPushButton::clicked, this, &AdminPage::onRenameUser);
    connect(departmentButton, &QPushButton::clicked, this, &AdminPage::onSetDepartment);
    connect(removeButton, &QPushButton::clicked, this, &AdminPage::onRemoveUser);
    return panel;
}

QWidget *AdminPage::buildCategoryPanel() {
    QWidget *panel = new QWidget(this);

    categoryTree = new QTreeWidget(panel);
    categoryTree->setHeaderLabel("设备分类层次");

    QPushButton *addRootButton = new QPushButton("新增顶层分类", panel);
    QPushButton *addChildButton = new QPushButton("新增子分类", panel);
    QPushButton *removeButton = new QPushButton("删除分类", panel);
    QPushButton *refreshButton = new QPushButton("刷新", panel);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(addRootButton);
    buttons->addWidget(addChildButton);
    buttons->addWidget(removeButton);
    buttons->addWidget(refreshButton);
    buttons->addStretch();

    categoryHint = new QLabel("新增子分类前先选中父分类；分类或其子分类下还有设备时不能删除", panel);

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->addWidget(categoryTree);
    layout->addLayout(buttons);
    layout->addWidget(categoryHint);

    connect(addRootButton, &QPushButton::clicked, this, &AdminPage::onAddRootCategory);
    connect(addChildButton, &QPushButton::clicked, this, &AdminPage::onAddChildCategory);
    connect(removeButton, &QPushButton::clicked, this, &AdminPage::onRemoveCategory);
    connect(refreshButton, &QPushButton::clicked, this, &AdminPage::reloadCategories);
    return panel;
}

QWidget *AdminPage::buildDevicePanel() {
    QWidget *panel = new QWidget(this);

    deviceTable = new QTableWidget(panel);
    deviceTable->setColumnCount(5);
    deviceTable->setHorizontalHeaderLabels({"编号", "名称", "分类", "位置", "状态"});
    deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceTable->verticalHeader()->setVisible(false);
    deviceTable->horizontalHeader()->setStretchLastSection(true);

    QPushButton *addButton = new QPushButton("新增设备", panel);
    QPushButton *updateButton = new QPushButton("修改设备", panel);
    QPushButton *removeButton = new QPushButton("删除设备", panel);
    QPushButton *refreshButton = new QPushButton("刷新", panel);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(addButton);
    buttons->addWidget(updateButton);
    buttons->addWidget(removeButton);
    buttons->addWidget(refreshButton);
    buttons->addStretch();

    deviceHint = new QLabel("新增设备前必须先建立分类；还有预约或已借出的设备不能删除", panel);

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->addWidget(deviceTable);
    layout->addLayout(buttons);
    layout->addWidget(deviceHint);

    connect(addButton, &QPushButton::clicked, this, &AdminPage::onAddDevice);
    connect(updateButton, &QPushButton::clicked, this, &AdminPage::onUpdateDevice);
    connect(removeButton, &QPushButton::clicked, this, &AdminPage::onRemoveDevice);
    connect(refreshButton, &QPushButton::clicked, this, &AdminPage::reloadDevices);
    return panel;
}

void AdminPage::refresh() {
    reloadUsers();
    reloadCategories();
    reloadDevices();
}

void AdminPage::reloadUsers() {
    int previousId = selectedUserId();

    userTable->setSortingEnabled(false);
    userTable->setRowCount(0);

    service.users().forEach([this](const User &user) {
        int row = userTable->rowCount();
        userTable->insertRow(row);
        userTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.getId())));
        userTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(user.getName())));
        userTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(user.getDepartment())));
    });

    userTable->setSortingEnabled(true);
    selectRowById(userTable, previousId);
}

void AdminPage::reloadCategories() {
    int previousNode = selectedCategoryId();

    categoryTree->clear();
    QHash<int, QTreeWidgetItem *> items;
    service.devices().forEachCategory([&items, this](const std::string &name, int nodeId, int parentId) {
        QTreeWidgetItem *item = nullptr;
        if (parentId == -1) item = new QTreeWidgetItem(categoryTree);
        else {
            QTreeWidgetItem *parent = items.value(parentId, nullptr);
            if (!parent) return;
            item = new QTreeWidgetItem(parent);
        }

        item->setText(0, QString::fromStdString(name));
        item->setData(0, CategoryIdRole, nodeId);
        items.insert(nodeId, item);
    });
    categoryTree->expandAll();

    if (previousNode >= 0) categoryTree->setCurrentItem(items.value(previousNode, nullptr));
}

void AdminPage::reloadDevices() {
    int previousId = selectedDeviceId();

    deviceTable->setSortingEnabled(false);
    deviceTable->setRowCount(0);

    service.devices().forEachDevice([this](const Device &device) {
        int row = deviceTable->rowCount();
        deviceTable->insertRow(row);
        deviceTable->setItem(row, 0, new QTableWidgetItem(QString::number(device.getId())));
        deviceTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(device.getName())));
        deviceTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(device.getCategory())));
        deviceTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(device.getLocation())));
        deviceTable->setItem(row, 4, new QTableWidgetItem(deviceStatusText(device.getStatus())));
    });

    deviceTable->setSortingEnabled(true);
    selectRowById(deviceTable, previousId);
}

int AdminPage::selectedUserId() const {
    int row = userTable->currentRow();
    if (row < 0) return -1;

    QTableWidgetItem *item = userTable->item(row, 0);
    return item ? item->text().toInt() : -1;
}

int AdminPage::selectedDeviceId() const {
    int row = deviceTable->currentRow();
    if (row < 0) return -1;

    QTableWidgetItem *item = deviceTable->item(row, 0);
    return item ? item->text().toInt() : -1;
}

int AdminPage::selectedCategoryId() const {
    QTreeWidgetItem *item = categoryTree->currentItem();
    if (!item) return -1;

    return item->data(0, CategoryIdRole).toInt();
}

void AdminPage::onAddUser() {
    bool accepted = false;
    QString name = QInputDialog::getText(this, "新增用户", "姓名:", QLineEdit::Normal, "", &accepted);
    if (!accepted || name.trimmed().isEmpty()) return;

    QString department = QInputDialog::getText(this, "新增用户", "部门:", QLineEdit::Normal, "", &accepted);
    if (!accepted || department.trimmed().isEmpty()) return;

    int userId = 0;
    OpResult result = service.users().addUser(name.trimmed().toStdString(),
                                              department.trimmed().toStdString(), userId);
    if (result == OpResult::Ok) {
        userHint->setText(QString("已新增用户 %1（编号 %2）").arg(name.trimmed()).arg(userId));
    }
    else {
        userHint->setText(QString("新增失败：%1").arg(resultText(result)));
    }
    reloadUsers();
}

void AdminPage::onRenameUser() {
    int userId = selectedUserId();
    if (userId < 0) {
        userHint->setText("请先选中一个用户");
        return;
    }

    bool accepted = false;
    QString name = QInputDialog::getText(this, "修改姓名", "姓名:", QLineEdit::Normal, "", &accepted);
    if (!accepted || name.trimmed().isEmpty()) return;

    OpResult result = service.users().renameUser(userId, name.trimmed().toStdString());
    userHint->setText(result == OpResult::Ok ? "修改成功" : QString("修改失败：%1").arg(resultText(result)));
    reloadUsers();
}

void AdminPage::onSetDepartment() {
    int userId = selectedUserId();
    if (userId < 0) {
        userHint->setText("请先选中一个用户");
        return;
    }

    bool accepted = false;
    QString department = QInputDialog::getText(this, "修改部门", "部门:", QLineEdit::Normal, "", &accepted);
    if (!accepted || department.trimmed().isEmpty()) return;

    OpResult result = service.users().setDepartment(userId, department.trimmed().toStdString());
    userHint->setText(result == OpResult::Ok ? "修改成功" : QString("修改失败：%1").arg(resultText(result)));
    reloadUsers();
}

void AdminPage::onRemoveUser() {
    int userId = selectedUserId();
    if (userId < 0) {
        userHint->setText("请先选中一个用户");
        return;
    }

    OpResult result = service.removeUser(userId);
    if (result == OpResult::Ok) {
        userHint->setText("删除成功");
    }
    else if (result == OpResult::InvalidState) {
        userHint->setText("删除失败：该用户还有未取消的预约或未归还的设备");
    }
    else {
        userHint->setText(QString("删除失败：%1").arg(resultText(result)));
    }
    reloadUsers();
}

void AdminPage::onAddRootCategory() {
    bool accepted = false;
    QString name = QInputDialog::getText(this, "新增顶层分类", "分类名称:", QLineEdit::Normal, "", &accepted);
    if (!accepted || name.trimmed().isEmpty()) return;

    int nodeId = -1;
    OpResult result = service.devices().addCategory(-1, name.trimmed().toStdString(), nodeId);
    categoryHint->setText(result == OpResult::Ok ? "新增成功" : QString("新增失败：%1").arg(resultText(result)));
    reloadCategories();
}

void AdminPage::onAddChildCategory() {
    int parentId = selectedCategoryId();
    if (parentId < 0) {
        categoryHint->setText("请先选中父分类");
        return;
    }

    bool accepted = false;
    QString name = QInputDialog::getText(this, "新增子分类", "分类名称:", QLineEdit::Normal, "", &accepted);
    if (!accepted || name.trimmed().isEmpty()) return;

    int nodeId = -1;
    OpResult result = service.devices().addCategory(parentId, name.trimmed().toStdString(), nodeId);
    categoryHint->setText(result == OpResult::Ok ? "新增成功" : QString("新增失败：%1").arg(resultText(result)));
    reloadCategories();
}

void AdminPage::onRemoveCategory() {
    int nodeId = selectedCategoryId();
    if (nodeId < 0) {
        categoryHint->setText("请先选中要删除的分类");
        return;
    }

    OpResult result = service.devices().removeCategory(nodeId);
    if (result == OpResult::Ok) {
        categoryHint->setText("删除成功");
    }
    else if (result == OpResult::InvalidState) {
        categoryHint->setText("删除失败：该分类或其子分类下还有设备");
    }
    else {
        categoryHint->setText(QString("删除失败：%1").arg(resultText(result)));
    }
    reloadCategories();
}

void AdminPage::onAddDevice() {
    QStringList categories = allCategoryNames(service);
    if (categories.isEmpty()) {
        deviceHint->setText("请先在分类管理中建立分类");
        return;
    }

    DeviceDialog dialog(this);
    dialog.setCategories(categories, QString());
    if (dialog.exec() != QDialog::Accepted) return;

    int deviceId = 0;
    OpResult result = service.devices().addDevice(dialog.name().toStdString(),
                                                  dialog.category().toStdString(),
                                                  dialog.location().toStdString(), deviceId);
    if (result == OpResult::Ok) {
        deviceHint->setText(QString("已新增设备（编号 %1）").arg(deviceId));
    }
    else {
        deviceHint->setText(QString("新增失败：%1").arg(resultText(result)));
    }
    reloadDevices();
}

void AdminPage::onUpdateDevice() {
    int deviceId = selectedDeviceId();
    if (deviceId < 0) {
        deviceHint->setText("请先选中一台设备");
        return;
    }

    Device device(0, "", "", "");  // 仅作接收容器
    if (!service.devices().findDevice(deviceId, device)) {
        deviceHint->setText("设备不存在");
        return;
    }

    DeviceDialog dialog(this);
    dialog.setCategories(allCategoryNames(service), QString::fromStdString(device.getCategory()));
    dialog.setDevice(QString::fromStdString(device.getName()),
                     QString::fromStdString(device.getLocation()));
    if (dialog.exec() != QDialog::Accepted) return;

    OpResult result = service.devices().updateDevice(deviceId, dialog.name().toStdString(),
                                                     dialog.category().toStdString(),
                                                     dialog.location().toStdString());
    deviceHint->setText(result == OpResult::Ok ? "修改成功" : QString("修改失败：%1").arg(resultText(result)));
    reloadDevices();
}

void AdminPage::onRemoveDevice() {
    int deviceId = selectedDeviceId();
    if (deviceId < 0) {
        deviceHint->setText("请先选中一台设备");
        return;
    }

    OpResult result = service.removeDevice(deviceId);
    if (result == OpResult::Ok) {
        deviceHint->setText("删除成功");
    }
    else if (result == OpResult::InvalidState) {
        deviceHint->setText("删除失败：该设备还有预约或处于借出状态");
    }
    else {
        deviceHint->setText(QString("删除失败：%1").arg(resultText(result)));
    }
    reloadDevices();
}
