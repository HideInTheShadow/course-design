#include <QApplication>

#include "gui/MainWindow.h"
#include "service/LabService.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 业务数据由 LabService 统一持有，界面只通过它读写
    LabService service;

    MainWindow window(service);
    window.show();

    return app.exec();
}
