#include "gui/MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("实验室共享设备预约与冲突调度系统");
    resize(1000, 680);
}
