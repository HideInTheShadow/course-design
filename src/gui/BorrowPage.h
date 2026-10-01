#ifndef BORROWPAGE_H
#define BORROWPAGE_H

#include <QWidget>

class QTableWidget;

class LabService;

// 借用历史页：按借用先后列出全部借用记录
class BorrowPage : public QWidget {
    Q_OBJECT

public:
    explicit BorrowPage(LabService &service, QWidget *parent = nullptr);

    void refresh();

private:
    LabService &service;

    QTableWidget *historyTable;
};

#endif
