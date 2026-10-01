#ifndef RESERVATIONPAGE_H
#define RESERVATIONPAGE_H

#include <QWidget>

class QLabel;
class QTableWidget;

class LabService;

// 预约页：查看全部预约、取消预约、撤销取消，并查看等待任务的排队顺序
class ReservationPage : public QWidget {
    Q_OBJECT

public:
    explicit ReservationPage(LabService &service, QWidget *parent = nullptr);

    void refresh();

private:
    int selectedReservationId() const;

    void reloadReservations();
    void reloadWaiting();
    void onCancelReservation();
    void onUndoCancel();

    LabService &service;

    QTableWidget *reservationTable;
    QTableWidget *waitingTable;
    QLabel *hintLabel;
};

#endif
