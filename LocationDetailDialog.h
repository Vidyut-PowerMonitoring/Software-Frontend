// LocationDetailDialog.h
#ifndef LOCATIONDETAILDIALOG_H
#define LOCATIONDETAILDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QGridLayout>
#include "ModernGaugeWidget.h"
#include "ScheduleManagerDialog.h"
#include <QtWebSockets/QWebSocket>
#include <QLabel>
#include <QPushButton>

class LocationDetailDialog : public QDialog
{
    Q_OBJECT

public:

    LocationDetailDialog(QString& name,QString& topic,QWebSocket* socket, QWidget *parent = nullptr);

    void setUI();


protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void handleSocketMessage(const QString& message);
    void toggleDevice(bool checked);
    void sendDeviceCommand(const QString &command);

private:
    QWebSocket* m_socket;
    QString m_name;
    QString m_topic;
    QGridLayout *m_gaugeLayout;
    QList<ModernGaugeWidget*> m_gauges;
    QTimer *m_timer;

    QPushButton *m_deviceToggleButton;
    QLabel *m_deviceStatusLabel;
    bool m_deviceRunning = false;
};

#endif // LOCATIONDETAILDIALOG_H


