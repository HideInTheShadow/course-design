#include "gui/MainWindow.h"

#include <QTabWidget>

#include "gui/DevicePage.h"
#include "service/LabService.h"

MainWindow::MainWindow(LabService &service, QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("实验室共享设备预约与冲突调度系统");
    resize(1200, 760);

    tabs = new QTabWidget(this);
    devicePage = new DevicePage(service, tabs);
    tabs->addTab(devicePage, "设备与预约");
    setCentralWidget(tabs);

    // 切到哪个页面就刷新哪个页面，保证各页显示同一份最新数据
    connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
        QWidget *page = tabs->widget(index);
        if (page == devicePage) devicePage->refresh();
    });
}
