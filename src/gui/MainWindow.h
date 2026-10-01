#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTabWidget;
class AdminPage;
class BorrowPage;
class DevicePage;
class ReservationPage;
class LabService;

// 主窗口只负责组织各功能页面与切换刷新，不承担业务逻辑
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(LabService &service, QWidget *parent = nullptr);

private:
    QTabWidget *tabs;
    DevicePage *devicePage;
    ReservationPage *reservationPage;
    BorrowPage *borrowPage;
    AdminPage *adminPage;
};

#endif
