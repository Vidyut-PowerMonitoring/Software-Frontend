#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QLineSeries>
#include <QChart>
#include <QChartView>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProgressBar>
#include <QProcess>
#include <QDialog>
#include <QStackedWidget>
#include <QComboBox>
#include <QtWebSockets/QWebSocket>
#include <QMap>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QTextEdit>
#include <QDateTime>
#include <QToolTip>
#include "LocationDetailDialog.h"
#include "ModernGaugeWidget.h"
#include "ScheduleManagerDialog.h"
#include "datarecorddialog.h"
//QT_CHARTS_USE_NAMESPACE
class LocationStats : public QObject {
    Q_OBJECT
public:
    QString name;
    QColor color;
    QString topic;
    QWebSocket* socket;
    int dataPointCount = 0;
    const int MAX_DATA_POINTS = 100;
    QLineSeries* powerSeries;
    QLineSeries* p1;
    QLineSeries* p2;
    QLineSeries* p3;
    QLineSeries* v1;
    QLineSeries* v2;
    QLineSeries* v3;
    QLineSeries* c1;
    QLineSeries* c2;
    QLineSeries* c3;
    QLineSeries* voltageSeries;
    QLineSeries* currentSeries;
    QList<QDateTime> timestamps;

    LocationStats(QString name, QColor color, QString topic)
        : name(name), color(color), topic(topic)
    {
        powerSeries = new QLineSeries();
        p1 = new QLineSeries();
        p2 = new QLineSeries();
        p3 = new QLineSeries();

        voltageSeries = new QLineSeries();
        v1 = new QLineSeries();
        v2 = new QLineSeries();
        v3 = new QLineSeries();

        currentSeries = new QLineSeries();
        c1 = new QLineSeries();
        c2 = new QLineSeries();
        c3 = new QLineSeries();

        // Setup hover handlers for tooltip display
        setupSeriesHover(p1, "Power1");
        setupSeriesHover(p2, "Power2");
        setupSeriesHover(p3, "Power3");
        setupSeriesHover(powerSeries, "Power");
        setupSeriesHover(voltageSeries, "Voltage");
        setupSeriesHover(currentSeries, "Current");
    }

    QVector<ScheduleManagerDialog::Schedule> schedules;

    void addDataPoint(double power, double voltage, double current) {
        QDateTime currentTime = QDateTime::currentDateTime();
        timestamps.append(currentTime);

        dataPointCount++;
        qint64 timeMs = currentTime.toMSecsSinceEpoch();

        powerSeries->append(timeMs, power);
        voltageSeries->append(timeMs, voltage);
        currentSeries->append(timeMs, current);
    }




private:
    void setupSeriesHover(QLineSeries* series, const QString& seriesType) {
        connect(series, &QLineSeries::hovered, [this, series, seriesType](const QPointF &point, bool state) {
            if (state) {
                int index = qRound(point.x()) - 1;
                if (index >= 0 && index < timestamps.size()) {
                    double value = point.y();
                    QString timeStr = timestamps[index].toString("yyyy-MM-dd hh:mm:ss");
                    QString tooltipText = QString("%1\n%2: %3\nTime: %4")
                                              .arg(name)
                                              .arg(seriesType)
                                              .arg(value)
                                              .arg(timeStr);
                    QToolTip::showText(QCursor::pos(), tooltipText);
                }
            }
        });
    }


};

class Cluster : public QMainWindow
{
    Q_OBJECT
public:
    explicit Cluster(QWidget *parent = nullptr);
    ~Cluster();
    void setupClusterUI();
private slots:
    void updateStats();
    void showLocationDetails(int locationIndex);
    void onConnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void updateChartRanges();

public slots:
    void showDataRecorder(int locationIndex);
    void showScheduleManager(int locationIndex);
    void updateLocationSchedules(int locationIndex,
                            const QVector<ScheduleManagerDialog::Schedule>& schedules);

private:
    QMap<QString, LocationStats*> topics;
    QMap<QWebSocket*, QString> socketToTopic;
    QList<QWebSocket*> sockets;
    void connectToWebsockets();
    void createBuildingsSection();
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;
    // Buildings section
    QWidget* buildingsWidget;
    QVBoxLayout* buildingsLayout;
    // Charts and chart views
    QChart* powerChart;
    QChartView* powerChartView;
    QChart* voltageChart;
    QChartView* voltageChartView;
    QChart* currentChart;
    QChartView* currentChartView;
    QStackedWidget* chartStackWidget;
    QVector<LocationStats*> locationStats;
    QVector<QLabel*> locationLabels;
    QVector<LocationDetailDialog*> locationDialogs;
    // Series for different measurements
    QVector<QLineSeries*> powerSeries;
    QVector<QLineSeries*> voltageSeries;
    QVector<QLineSeries*> currentSeries;
    // Power section
    QWidget* powerWidget;
    QGridLayout* powerLayout;
    QProgressBar* voltageBar;
    QLabel* voltageLabel;
    QLabel* voltageUsageLabel;
    QLineSeries* voltageSingleSeries;
    // Current section
    QProgressBar* currentBar;
    QLabel* currentLabel;
    QLabel* currentUsageLabel;
    QLineSeries* currentSingleSeries;
    QTimer* updateTimer;
    QProcess* systemInfoProcess;
    // Data for simulating system stats
    int dataPointCount;
    const int MAX_DATA_POINTS = 100;

    // Time-axis related members
    QDateTimeAxis* powerTimeAxis;
    QDateTimeAxis* voltageTimeAxis;
    QDateTimeAxis* currentTimeAxis;

    // Update y-axis ranges based on current data
    void updateYAxisRanges();

    // for schedule manager
    QVector<QVector<ScheduleManagerDialog::Schedule>> locationSchedules;

};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    void setupUI();
    QVector<Cluster*> clusters;
    QVBoxLayout* clustersLayout;
};
#endif // MAINWINDOW_H
