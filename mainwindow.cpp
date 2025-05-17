#include <iostream>
#include "mainwindow.h"
#include <QRandomGenerator>
#include <QPalette>
#include <QDateTime>
#include <QDebug>
#include <QtWebSockets/QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextEdit>
#include "LocationDetailDialog.h"
#include "ModernGaugeWidget.h"
#include "ScheduleManagerDialog.h"

Cluster::Cluster(QWidget *parent)
    : QMainWindow(parent), dataPointCount(0)
{
    // Don't call setupUI() here anymore as it will be called by MainWindow
}

Cluster::~Cluster()
{
    for (auto series : powerSeries) {
        delete series;
    }

    for (auto series : voltageSeries){
        delete series;
    }

    for (auto series : currentSeries){
        delete series;
    }

    for (auto dialog : locationDialogs) {
        delete dialog;
    }
}

void Cluster::setupClusterUI()
{
    // Create a container widget
    QWidget* containerWidget = new QWidget(this);
    setCentralWidget(containerWidget);

    mainLayout = new QVBoxLayout(containerWidget);

    // Set dark theme
    setStyleSheet("QMainWindow { background-color: #1e1e1e; color: white; }"
                  "QWidget { background-color: #1e1e1e; color: white; }"
                  "QTabWidget::pane { border: 1px solid #444; }"
                  "QTabWidget::tab-bar { left: 5px; }"
                  "QTabBar::tab { background-color: #333; color: white; padding: 8px 12px; margin-right: 2px; }"
                  "QTabBar::tab:selected { background-color: #444; border-bottom-color: #444; }");

    // Setup tabs
    QTabWidget* tabWidget = new QTabWidget();
    tabWidget->setStyleSheet("background-color: #1e1e1e;");

    // Create all sections and connect to webSockets
    createBuildingsSection();
    connectToWebsockets();

    // Add widgets to tabs
    tabWidget->addTab(buildingsWidget, "NODE");

    mainLayout->addWidget(tabWidget);

    // Set a fixed height to make scrolling useful
    setFixedHeight(600);
}

void Cluster::showDataRecorder(int locationIndex)
{
    if (locationIndex < 0 || locationIndex >= locationStats.size())
        return;

    DataRecordDialog* dialog = new DataRecordDialog(
        this,
        locationIndex,
        locationStats[locationIndex]->name,
        locationStats[locationIndex]->color,
        locationStats[locationIndex]->topic,
        locationStats[locationIndex]->socket
        );

    // Set dialog to delete itself when closed
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Show the dialog modally
    dialog->exec();
}

// Add this method to your main class (in your .cpp file)
void Cluster::showScheduleManager(int locationIndex)
{
    if (locationIndex < 0 || locationIndex >= locationStats.size())
        return;

    ScheduleManagerDialog* dialog = new ScheduleManagerDialog(
        this,
        locationIndex,
        locationStats[locationIndex]->name,
        locationStats[locationIndex]->color,
        locationStats[locationIndex]->topic,
        locationStats[locationIndex]->socket
        );

    // Connect the signal to handle schedule changes
    connect(dialog, &ScheduleManagerDialog::schedulesChanged,
            this, &Cluster::updateLocationSchedules);

    // Set dialog to delete itself when closed
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Show the dialog modally
    dialog->exec();
}

// Also add this method to handle schedule updates
void Cluster::updateLocationSchedules(int locationIndex,
                                            const QVector<ScheduleManagerDialog::Schedule>& schedules)
{
    if (locationIndex < 0 || locationIndex >= locationStats.size())
        return;

    // Store the schedules in your data model
    // If you don't already have a schedules member in your locationStats struct,
    // you may need to add one:
    locationStats[locationIndex]->schedules = schedules;

    // If you have a separate data structure for schedules:
    // locationSchedules[locationIndex] = schedules;

    // Update any UI elements that show schedule status
    // updateLocationDisplay(locationIndex);

    // // Save changes to settings or database
    // saveSettings();
}
void Cluster::createBuildingsSection()
{
    buildingsWidget = new QWidget();
    buildingsLayout = new QVBoxLayout(buildingsWidget);

    // Add dropdown for selecting series type
    QHBoxLayout* controlLayout = new QHBoxLayout();
    QLabel* seriesLabel = new QLabel("Select Series:");
    seriesLabel->setStyleSheet("color: white;");

    QComboBox* seriesSelector = new QComboBox();
    seriesSelector->addItem("Power");
    seriesSelector->addItem("Voltage");
    seriesSelector->addItem("Current");
    seriesSelector->setStyleSheet("background-color: #333; color: white; padding: 5px;");

    controlLayout->addWidget(seriesLabel);
    controlLayout->addWidget(seriesSelector);
    controlLayout->addStretch();

    buildingsLayout->addLayout(controlLayout);

    // Initialize location stats with colors
    QColor colors[] = {
        Qt::green, Qt::red, Qt::blue
    };

    // Create building/location entries   from DB
    // QStringList topicNames={"topic1","topic2","topic3"};
    QStringList topicNames={"modbus/data","modbus/registers","topic2"};
    for (int i = 0; i < 3; ++i) {
        QString topic = topicNames[i];
        auto* Location = new LocationStats(QString("Building %1").arg(i+1), colors[i % 3], topic);
        locationStats.append(Location);
        topics[topic] = Location;
    }


    QGridLayout* locationGridLayout = new QGridLayout();

    for (int i = 0; i < locationStats.size(); ++i) {
        QLabel* locationLabel = new QLabel(QString("%1").arg(locationStats[i]->name));
        QPalette palette = locationLabel->palette();
        palette.setColor(QPalette::WindowText, locationStats[i]->color);
        locationLabel->setPalette(palette);
        locationLabels.append(locationLabel);

        // Location Button
        QPushButton* locationButton = new QPushButton(locationStats[i]->name);
        locationButton->setStyleSheet(QString("background-color: %1; color: white;").arg(locationStats[i]->color.name()));
        connect(locationButton, &QPushButton::clicked, this, [this, i]() { showLocationDetails(i); });

        // Timer Button
        QPushButton* timerButton = new QPushButton("Set Timer");
        timerButton->setStyleSheet("background-color: #2980b9; color: white;");
        connect(timerButton, &QPushButton::clicked, this, [this, i]() { showScheduleManager(i); });

        // Record Data Button (new)
        QPushButton* recordButton = new QPushButton("Record Data");
        recordButton->setStyleSheet("background-color: #e74c3c; color: white;");
        connect(recordButton, &QPushButton::clicked, this, [this, i]() { showDataRecorder(i); });

        // Create a horizontal layout for buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(locationButton);
        buttonLayout->addWidget(timerButton);
        buttonLayout->addWidget(recordButton); // Add the new button

        // Create a widget to hold the buttons layout
        QWidget* buttonWidget = new QWidget();
        buttonWidget->setLayout(buttonLayout);

        int row = 1;
        int col = i;

        // Add buttons side by side
        locationGridLayout->addWidget(buttonWidget, row, col * 2);
        // Add label to the right of buttons
        locationGridLayout->addWidget(locationLabel, row, col * 2 + 1);
    }



    buildingsLayout->addLayout(locationGridLayout);

    // Create stack widget to hold different charts
    chartStackWidget = new QStackedWidget();

    // Create power usage chart
    powerChart = new QChart();
    powerChart->setTitle("Power Usage");
    powerChart->legend()->hide();
    powerChart->setBackgroundBrush(QBrush(QColor("#1e1e1e")));
    powerChart->setTitleBrush(QBrush(QColor("white")));

    // Use QDateTimeAxis for X axis instead of QValueAxis
    QDateTime startTime = QDateTime::currentDateTime();
    QDateTime endTime = startTime.addSecs(MAX_DATA_POINTS);

    // Power chart time axis
    powerTimeAxis = new QDateTimeAxis();
    // powerTimeAxis->setRange(startTime, endTime);
    powerTimeAxis->setFormat("hh:mm:ss");
    powerTimeAxis->setTitleText("Time");
    powerTimeAxis->setLabelsColor(QColor("white"));
    powerTimeAxis->setTitleBrush(QBrush(QColor("white")));

    QValueAxis* powerAxisY = new QValueAxis();
    powerAxisY->setRange(0, 100);
    powerAxisY->setLabelFormat("%d");
    powerAxisY->setTitleText("Power (W)");
    powerAxisY->setLabelsColor(QColor("white"));
    powerAxisY->setTitleBrush(QBrush(QColor("white")));

    powerChart->addAxis(powerTimeAxis, Qt::AlignBottom);
    powerChart->addAxis(powerAxisY, Qt::AlignLeft);

    // Create series for each location - power
    for (int i = 0; i < locationStats.size(); ++i) {
        QLineSeries* series = locationStats[i]->powerSeries;
        series->setName(locationStats[i]->name);
        series->setPen(QPen(locationStats[i]->color, 2));
        powerChart->addSeries(series);
        series->attachAxis(powerTimeAxis);
        series->attachAxis(powerAxisY);
        powerSeries.append(series);
    }

    for (int i = 0; i < locationStats.size(); ++i) {
        QLineSeries* series1 = locationStats[i]->p1;
        series1->setName(locationStats[i]->name);
        series1->setPen(QPen(locationStats[i]->color, 2));
        powerChart->addSeries(series1);
        series1->attachAxis(powerTimeAxis);
        series1->attachAxis(powerAxisY);
        powerSeries.append(series1);

        QLineSeries* series2 = locationStats[i]->p2;
        series2->setName(locationStats[i]->name);
        series2->setPen(QPen(locationStats[i]->color, 2));
        powerChart->addSeries(series2);
        series2->attachAxis(powerTimeAxis);
        series2->attachAxis(powerAxisY);
        powerSeries.append(series2);

        QLineSeries* series3 = locationStats[i]->p3;
        series3->setName(locationStats[i]->name);
        series3->setPen(QPen(locationStats[i]->color, 2));
        powerChart->addSeries(series3);
        series3->attachAxis(powerTimeAxis);
        series3->attachAxis(powerAxisY);
        powerSeries.append(series3);
    }

    powerChartView = new QChartView(powerChart);
    powerChartView->setRenderHint(QPainter::Antialiasing);
    powerChartView->setStyleSheet("background-color: #1e1e1e;");

    // Create voltage chart
    voltageChart = new QChart();
    voltageChart->setTitle("Voltage");
    voltageChart->legend()->hide();
    voltageChart->setBackgroundBrush(QBrush(QColor("#1e1e1e")));
    voltageChart->setTitleBrush(QBrush(QColor("white")));

    // Voltage chart time axis
    voltageTimeAxis = new QDateTimeAxis();
    voltageTimeAxis->setRange(startTime, endTime);
    voltageTimeAxis->setFormat("hh:mm:ss");
    voltageTimeAxis->setTitleText("Time");
    voltageTimeAxis->setLabelsColor(QColor("white"));
    voltageTimeAxis->setTitleBrush(QBrush(QColor("white")));

    QValueAxis* voltageAxisY = new QValueAxis();
    // voltageAxisY->setRange(0, 100);
    voltageAxisY->setLabelFormat("%d");
    voltageAxisY->setTitleText("Voltage (V)");
    voltageAxisY->setLabelsColor(QColor("white"));
    voltageAxisY->setTitleBrush(QBrush(QColor("white")));

    voltageChart->addAxis(voltageTimeAxis, Qt::AlignBottom);
    voltageChart->addAxis(voltageAxisY, Qt::AlignLeft);

    // Create series for each location - voltage
    for (int i = 0; i < locationStats.size(); ++i) {
        QLineSeries* series = locationStats[i]->voltageSeries;
        series->setName(locationStats[i]->name);
        series->setPen(QPen(locationStats[i]->color, 2));
        voltageChart->addSeries(series);
        series->attachAxis(voltageTimeAxis);
        series->attachAxis(voltageAxisY);
        voltageSeries.append(series);
    }

    for (int i = 0; i < locationStats.size(); ++i) {
        QLineSeries* series1 = locationStats[i]->v1;
        series1->setName(locationStats[i]->name);
        series1->setPen(QPen(locationStats[i]->color, 2));
        voltageChart->addSeries(series1);
        series1->attachAxis(voltageTimeAxis);
        series1->attachAxis(voltageAxisY);
        voltageSeries.append(series1);

        QLineSeries* series2 = locationStats[i]->v2;
        series2->setName(locationStats[i]->name);
        series2->setPen(QPen(locationStats[i]->color, 2));
        voltageChart->addSeries(series2);
        series2->attachAxis(voltageTimeAxis);
        series2->attachAxis(voltageAxisY);
        voltageSeries.append(series2);

        QLineSeries* series3 = locationStats[i]->v3;
        series3->setName(locationStats[i]->name);
        series3->setPen(QPen(locationStats[i]->color, 2));
        voltageChart->addSeries(series3);
        series3->attachAxis(voltageTimeAxis);
        series3->attachAxis(voltageAxisY);
        voltageSeries.append(series3);
    }

    voltageChartView = new QChartView(voltageChart);
    voltageChartView->setRenderHint(QPainter::Antialiasing);
    voltageChartView->setStyleSheet("background-color: #1e1e1e;");

    // Create current chart
    currentChart = new QChart();
    currentChart->setTitle("Current");
    currentChart->legend()->hide();
    currentChart->setBackgroundBrush(QBrush(QColor("#1e1e1e")));
    currentChart->setTitleBrush(QBrush(QColor("white")));

    // Current chart time axis
    currentTimeAxis = new QDateTimeAxis();
    // currentTimeAxis->setRange(startTime, endTime);
    currentTimeAxis->setFormat("hh:mm:ss");
    currentTimeAxis->setTitleText("Time");
    currentTimeAxis->setLabelsColor(QColor("white"));
    currentTimeAxis->setTitleBrush(QBrush(QColor("white")));

    QValueAxis* currentAxisY = new QValueAxis();
    currentAxisY->setRange(0, 100);
    currentAxisY->setLabelFormat("%d");
    currentAxisY->setTitleText("Current (A)");
    currentAxisY->setLabelsColor(QColor("white"));
    currentAxisY->setTitleBrush(QBrush(QColor("white")));

    currentChart->addAxis(currentTimeAxis, Qt::AlignBottom);
    currentChart->addAxis(currentAxisY, Qt::AlignLeft);

    // Create series for each location - current
    for (int i = 0; i < locationStats.size(); ++i) {
        QLineSeries* series = locationStats[i]->currentSeries;
        series->setName(locationStats[i]->name);
        series->setPen(QPen(locationStats[i]->color, 2));
        currentChart->addSeries(series);
        series->attachAxis(currentTimeAxis);
        series->attachAxis(currentAxisY);
        currentSeries.append(series);
    }

    for (int i = 0; i < locationStats.size(); ++i) {
        QLineSeries* series1 = locationStats[i]->c1;
        series1->setName(locationStats[i]->name);
        series1->setPen(QPen(locationStats[i]->color, 2));
        currentChart->addSeries(series1);
        series1->attachAxis(currentTimeAxis);
        series1->attachAxis(currentAxisY);
        currentSeries.append(series1);

        QLineSeries* series2 = locationStats[i]->c2;
        series2->setName(locationStats[i]->name);
        series2->setPen(QPen(locationStats[i]->color, 2));
        currentChart->addSeries(series2);
        series2->attachAxis(currentTimeAxis);
        series2->attachAxis(currentAxisY);
        currentSeries.append(series2);

        QLineSeries* series3 = locationStats[i]->c3;
        series3->setName(locationStats[i]->name);
        series3->setPen(QPen(locationStats[i]->color, 2));
        currentChart->addSeries(series3);
        series3->attachAxis(currentTimeAxis);
        series3->attachAxis(currentAxisY);
        currentSeries.append(series3);
    }

    currentChartView = new QChartView(currentChart);
    currentChartView->setRenderHint(QPainter::Antialiasing);
    currentChartView->setStyleSheet("background-color: #1e1e1e;");

    // Add chart views to stack widget
    chartStackWidget->addWidget(powerChartView);
    chartStackWidget->addWidget(voltageChartView);
    chartStackWidget->addWidget(currentChartView);

    // Connect combo box to switch charts
    connect(seriesSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            chartStackWidget, &QStackedWidget::setCurrentIndex);

    buildingsLayout->addWidget(chartStackWidget);
}

void Cluster::onConnected() {
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (socket) {
        QString topic = socketToTopic.value(socket);
        qDebug() << "WebSocket connected to server for topic:" << topic;
    }
}

void Cluster::onTextMessageReceived(const QString &message) {
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    QString topic = socketToTopic.value(socket);
    if (topic.isEmpty()) return;

    qDebug() << "Received on" << topic << ":" << message;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Error: Invalid JSON message received";
        return;
    }

    QJsonObject obj = doc.object();

    if (obj.contains("v1") && obj.contains("v2") && obj.contains("v3") && obj.contains("c1") && obj.contains("c2") && obj.contains("c3") && obj.contains("p1") && obj.contains("p2") && obj.contains("p3")) {
        double v1 = obj["v1"].toDouble();
        double v2 = obj["v2"].toDouble();
        double v3 = obj["v3"].toDouble();
        double c1 = obj["c1"].toDouble();
        double c2 = obj["c2"].toDouble();
        double c3 = obj["c3"].toDouble();
        double p1 = obj["p1"].toDouble();
        double p2 = obj["p2"].toDouble();
        double p3 = obj["p3"].toDouble();
        double voltage = v3;
        double current = c3;
        double power = p3;

        LocationStats* location = topics[topic];
        QDateTime currentTime = QDateTime::currentDateTime();

        // Store timestamp
        location->timestamps.append(currentTime);
        location->dataPointCount++;
        dataPointCount = qMax(location->dataPointCount, dataPointCount);

        // Convert timestamp to msecsSinceEpoch for QLineSeries
        qint64 timeMs = currentTime.toMSecsSinceEpoch();

        // Add data point with timestamp as x-value
        location->powerSeries->append(timeMs, power);
        location->p1->append(timeMs, p1);
        location->p2->append(timeMs, p2);
        location->p3->append(timeMs, p3);
        location->voltageSeries->append(timeMs, voltage);
        location->v1->append(timeMs, v1);
        location->v2->append(timeMs, v2);
        location->v3->append(timeMs, v3);
        location->currentSeries->append(timeMs, current);
        location->c1->append(timeMs, c1);
        location->c2->append(timeMs, c2);
        location->c3->append(timeMs, c3);

        // Remove oldest points if we exceed MAX_DATA_POINTS
        if (location->timestamps.size() > location->MAX_DATA_POINTS) {
            location->powerSeries->remove(0);
            location->p1->remove(0);
            location->p2->remove(0);
            location->p3->remove(0);
            location->voltageSeries->remove(0);
            location->v1->remove(0);
            location->v2->remove(0);
            location->v3->remove(0);
            location->currentSeries->remove(0);
            location->c1->remove(0);
            location->c2->remove(0);
            location->c3->remove(0);

            location->timestamps.removeFirst();
        }

        // Update chart ranges
        updateChartRanges();

        // Update charts
        powerChartView->update();
        voltageChartView->update();
        currentChartView->update();
    } else {
        qDebug() << "Error: Missing voltage/current/power in JSON data";
    }
}

void Cluster::updateChartRanges() {
    // Get the earliest and latest timestamps from all locations
    QDateTime earliestTime;
    QDateTime latestTime;
    bool first = true;

    for (LocationStats* location : locationStats) {
        if (!location->timestamps.isEmpty()) {
            QDateTime locationEarliest = location->timestamps.first();
            QDateTime locationLatest = location->timestamps.last();

            if (first || locationEarliest < earliestTime) {
                earliestTime = locationEarliest;
            }

            if (first || locationLatest > latestTime) {
                latestTime = locationLatest;
            }

            if (first) {
                first = false;
            }
        }
    }

    // If we have valid timestamps, update the ranges
    if (!first) {
        // Add a small buffer at the end for better visibility
        latestTime = latestTime.addSecs(5);

        // Make sure we have a reasonable time window if not enough data
        if (earliestTime.secsTo(latestTime) < 30) {
            earliestTime = latestTime.addSecs(-30);
        }

        // Update all time axes
        powerTimeAxis->setRange(earliestTime, latestTime);
        voltageTimeAxis->setRange(earliestTime, latestTime);
        currentTimeAxis->setRange(earliestTime, latestTime);

        // Also update Y axis ranges based on data
        updateYAxisRanges();
    }
}

void Cluster::updateYAxisRanges() {
    // Find max values for power, voltage, and current
    double maxPower = 0;
    double maxVoltage = 0;
    double maxCurrent = 0;

    for (LocationStats* location : locationStats) {
        // Check power series
        const QList<QPointF>& powerPoints = location->powerSeries->points();
        for (const QPointF& point : powerPoints) {
            maxPower = qMax(maxPower, point.y());
        }

        // Check voltage series
        const QList<QPointF>& voltagePoints = location->voltageSeries->points();
        for (const QPointF& point : voltagePoints) {
            maxVoltage = qMax(maxVoltage, point.y());
        }

        // Check current series
        const QList<QPointF>& currentPoints = location->currentSeries->points();
        for (const QPointF& point : currentPoints) {
            maxCurrent = qMax(maxCurrent, point.y());
        }
    }

    // Add 10% margin to max values
    maxPower *= 1.1;
    maxVoltage *= 1.1;
    maxCurrent *= 1.1;

    // Set minimums
    maxPower = qMax(100.0, maxPower);
    maxVoltage = qMax(100.0, maxVoltage);
    maxCurrent = qMax(100.0, maxCurrent);

    // Update axes
    QValueAxis* powerAxisY = qobject_cast<QValueAxis*>(powerChart->axisY());
    QValueAxis* voltageAxisY = qobject_cast<QValueAxis*>(voltageChart->axisY());
    QValueAxis* currentAxisY = qobject_cast<QValueAxis*>(currentChart->axisY());

    powerAxisY->setRange(0, maxPower);
    voltageAxisY->setRange(0, maxVoltage);
    currentAxisY->setRange(0, maxCurrent);
}


void Cluster::onError(QAbstractSocket::SocketError error) {
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (socket) {
        QString topic = socketToTopic.value(socket);
        qDebug() << "WebSocket error on topic:" << topic;
    }
}

void Cluster::connectToWebsockets() {
    for (const QString& topic : topics.keys()) {
        QString wsUrl = QString("ws://localhost:8080/ws/%1").arg(topic);

        QWebSocket *socket = new QWebSocket();
        connect(socket, &QWebSocket::connected, this, &Cluster::onConnected);
        connect(socket, &QWebSocket::textMessageReceived, this, &Cluster::onTextMessageReceived);
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &Cluster::onError);

        socketToTopic[socket] = topic;
        sockets.append(socket);

        LocationStats* location = topics[topic];
        location->socket = socket;

        socket->open(QUrl(wsUrl));
    }
}

void Cluster::updateStats()
{
    dataPointCount++;
    QDateTime currentTime = QDateTime::currentDateTime();

    // Update location stats with simulated data
    for (int i = 0; i < locationStats.size(); ++i) {
        // Generate random power usage
        int powerUsage = QRandomGenerator::global()->bounded(20);
        int voltageValue = QRandomGenerator::global()->bounded(70, 90);  // 70-90
        int currentValue = QRandomGenerator::global()->bounded(10, 50);  // 10-50

        if (QRandomGenerator::global()->bounded(10) == 0) { // 10% chance of a spike
            powerUsage = QRandomGenerator::global()->bounded(50, 100);
            voltageValue = QRandomGenerator::global()->bounded(90, 110);  // 90-110
            currentValue = QRandomGenerator::global()->bounded(50, 80);   // 50-80
        }

        locationLabels[i]->setText(QString("%1 ").arg(locationStats[i]->name));

        // Store timestamp for this data point
        locationStats[i]->timestamps.append(currentTime);

        // Add data point to chart
        powerSeries[i]->append(dataPointCount, powerUsage);
        voltageSeries[i]->append(dataPointCount, voltageValue);
        currentSeries[i]->append(dataPointCount, currentValue);

        // Remove old data points if we have too many
        if (powerSeries[i]->count() > MAX_DATA_POINTS) {
            powerSeries[i]->remove(0);
            voltageSeries[i]->remove(0);
            currentSeries[i]->remove(0);

            // Also remove the oldest timestamp
            if (!locationStats[i]->timestamps.isEmpty()) {
                locationStats[i]->timestamps.removeFirst();
            }
        }
    }

    // Update chart time ranges
    updateChartRanges();

    // Update chart views
    powerChartView->update();
    voltageChartView->update();
    currentChartView->update();
}

void Cluster::showLocationDetails(int locationIndex)
{
    LocationStats* location = locationStats[locationIndex];

    // Create and show the ModernGaugeDialog
    LocationDetailDialog* dialog = new LocationDetailDialog(location->name, location->topic, location->socket);

    // Set the dialog title to include location name
    dialog->setWindowTitle(QString("Power Monitoring - %1").arg(location->name));

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    dialog->exec();
}

void MainWindow::setupUI()
{
    // Create a central widget for MainWindow
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Create a main layout for the central widget
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Create a scroll area for multiple clusters
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Create a widget to hold all clusters in the scroll area
    QWidget* scrollWidget = new QWidget();
    clustersLayout = new QVBoxLayout(scrollWidget);

    // Create multiple clusters (for example, 3 clusters)
    for (int i = 0; i < 2; i++) {
        Cluster* cluster = new Cluster();
        cluster->setWindowTitle(QString("Cluster %1").arg(i + 1));

        // We need to modify the Cluster to use its own setupUI, not setting itself as central widget
        cluster->setupClusterUI();

        // Add the cluster to our layout
        clustersLayout->addWidget(cluster);
        clusters.append(cluster);
    }

    // Add some spacing between clusters
    clustersLayout->setSpacing(20);
    clustersLayout->addStretch();

    // Set the scroll widget to the scroll area
    scrollArea->setWidget(scrollWidget);

    // Add the scroll area to the main layout
    mainLayout->addWidget(scrollArea);

    // Set stylesheet for MainWindow
    setStyleSheet("QMainWindow { background-color: #1e1e1e; color: white; }"
                  "QScrollArea { background-color: #1e1e1e; border: none; }"
                  "QScrollBar { background-color: #2d2d2d; }");
}

// New MainWindow class implementation
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Power Monitoring System");
    resize(1200, 800);

    setupUI();
}

MainWindow::~MainWindow()
{
    // No need to explicitly delete cluster since it's a child of this QMainWindow
    // and will be automatically deleted when the parent is destroyed
}
