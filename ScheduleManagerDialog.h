#ifndef SCHEDULEMANAGERDIALOG_H
#define SCHEDULEMANAGERDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTimeEdit>
#include <QDialogButtonBox>
#include <QColor>
#include <QVector>
#include <QTime>
#include <QtWebSockets/QWebSocket>
#include <QString>
#include <QNetworkAccessManager>

class ScheduleManagerDialog : public QDialog
{
    Q_OBJECT

public:
    ScheduleManagerDialog(QWidget* parent, int locationIndex, const QString& locationName, QColor locationColor,QString& topic,QWebSocket* socket );

    struct Schedule {
        QTime startTime;
        QTime endTime;
        bool isActive;
    };

signals:
    void schedulesChanged(int locationIndex, const QVector<ScheduleManagerDialog::Schedule>& schedules);

private slots:
    void addNewSchedule();
    void deleteSchedule();
    void toggleScheduleActive(int scheduleIndex, QPushButton* button);


private:
    void setupUI();
    void loadSchedules();
    void updateScheduleListItem(int scheduleIndex);
    void saveSchedules();
    void saveScheduleToDB(QTime, QTime);
    void deleteScheduleFromDB(QTime,QTime);

    int m_locationIndex;
    QString m_locationName;
    QColor m_locationColor;
    QVBoxLayout* mainLayout;
    QListWidget* scheduleListWidget;
    QString m_topic;
    QWebSocket* m_socket;
    QVector<Schedule> schedules;
    QNetworkAccessManager* manager;

};

#endif // SCHEDULEMANAGERDIALOG_H
