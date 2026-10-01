#include <QApplication>
#include <QDebug>

#include "gui/MainWindow.h"
#include "service/LabService.h"
#include "storage/DataStore.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 业务数据由 LabService 统一持有，界面只通过它读写
    LabService service;

    // 启动时把本地文件读进内存中的数据结构，退出前再写回
    DataStore store("lab_data.txt");
    store.load(service);
    if (store.skippedLines() > 0) {
        qWarning() << "数据文件中有" << store.skippedLines() << "条记录格式不正确，已跳过";
    }

    MainWindow window(service);
    window.show();

    int exitCode = app.exec();

    if (!store.save(service)) {
        qWarning() << "数据保存失败，文件路径:" << QString::fromStdString(store.filePath());
    }
    return exitCode;
}
