#include "gui/MainWindow.h"

#include <QTabWidget>

#include "gui/AdminPage.h"
#include "gui/BorrowPage.h"
#include "gui/DevicePage.h"
#include "gui/ReservationPage.h"
#include "service/LabService.h"

MainWindow::MainWindow(LabService &service, QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("实验室共享设备预约与冲突调度系统");
    resize(1200, 760);

    tabs = new QTabWidget(this);
    devicePage = new DevicePage(service, tabs);
    reservationPage = new ReservationPage(service, tabs);
    borrowPage = new BorrowPage(service, tabs);
    adminPage = new AdminPage(service, tabs);

    tabs->addTab(devicePage, "设备与预约");
    tabs->addTab(reservationPage, "预约与等待");
    tabs->addTab(borrowPage, "借用历史");
    tabs->addTab(adminPage, "管理");
    setCentralWidget(tabs);

    // 切到哪个页面就刷新哪个页面，保证各页显示同一份最新数据
    connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
        QWidget *page = tabs->widget(index);
        if (page == devicePage) devicePage->refresh();
        else if (page == reservationPage) reservationPage->refresh();
        else if (page == borrowPage) borrowPage->refresh();
        else if (page == adminPage) adminPage->refresh();
    });
}
