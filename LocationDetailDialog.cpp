// LocationDetailDialog.cpp
#include "LocationDetailDialog.h"
#include "ScheduleManagerDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QStyle>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextEdit>
#include <QtWebSockets/QWebSocket>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QPushButton>

LocationDetailDialog::LocationDetailDialog(QString& name ,QString& topic,QWebSocket* socket,QWidget *parent)
    : QDialog(parent)
{
    this->m_name = name;
    this->m_topic = topic;
    this->m_socket = socket;

    this->setUI();

    connect(m_socket, &QWebSocket::textMessageReceived, this, &LocationDetailDialog::handleSocketMessage);

    // Optional: disconnect on dialog close
    connect(this, &QDialog::finished, this, [this]() {
        disconnect(m_socket, &QWebSocket::textMessageReceived, this, &LocationDetailDialog::handleSocketMessage);
    });


}

void LocationDetailDialog::setUI() {
    setWindowTitle("Power Monitoring");
    setStyleSheet("background-color: #0F0F0F;");

    // Create layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    QLabel *titleLabel = new QLabel(this->m_name, this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 24px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    // Grid for gauges
    m_gaugeLayout = new QGridLayout();
    m_gaugeLayout->setSpacing(20);

    // Create phase labels
    QStringList phaseLabels = {"Phase 1", "Phase 2", "Phase 3"};
    for (int col = 0; col < 3; ++col) {
        QLabel *phaseLabel = new QLabel(phaseLabels[col], this);
        phaseLabel->setAlignment(Qt::AlignCenter);
        phaseLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: bold;");
        m_gaugeLayout->addWidget(phaseLabel, 0, col);
    }

    // Create row labels
    QStringList rowLabels = {"Voltage", "Current", "Power"};
    for (int row = 0; row < 3; ++row) {
        QLabel *rowLabel = new QLabel(rowLabels[row], this);
        rowLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        rowLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: bold;");
        m_gaugeLayout->addWidget(rowLabel, row + 1, 3);
    }

    // Create gauges
    QColor voltageColor(0, 255, 170);  // Turquoise like in the image
    QColor currentColor(255, 100, 100);  // Red
    QColor powerColor(100, 150, 255);    // Blue

    // Populate gauges
    for (int col = 0; col < 3; ++col) {
        // Voltage gauge
        ModernGaugeWidget *voltageGauge = new ModernGaugeWidget(
            "VOLTAGE", "", 0, 500, "V", voltageColor, this);
        m_gaugeLayout->addWidget(voltageGauge, 1, col);
        m_gauges.append(voltageGauge);

        // Current gauge
        ModernGaugeWidget *currentGauge = new ModernGaugeWidget(
            "CURRENT", "", 0, 100, "A", currentColor, this);
        m_gaugeLayout->addWidget(currentGauge, 2, col);
        m_gauges.append(currentGauge);

        // Power gauge
        ModernGaugeWidget *powerGauge = new ModernGaugeWidget(
            "POWER", "", 0, 5000, "W", powerColor, this);
        m_gaugeLayout->addWidget(powerGauge, 3, col);
        m_gauges.append(powerGauge);
    }

    mainLayout->addLayout(m_gaugeLayout);

    // Create a horizontal layout for device toggle controls
    QHBoxLayout *deviceControlLayout = new QHBoxLayout();
    deviceControlLayout->setSpacing(15);
    deviceControlLayout->setContentsMargins(0, 20, 0, 0);

    // Create device toggle button
    m_deviceToggleButton = new QPushButton("Turn Device ON", this);
    m_deviceToggleButton->setCheckable(true);
    m_deviceToggleButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    border: none;"
        "    color: white;"
        "    padding: 12px 25px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    border-radius: 8px;"
        "}"
        "QPushButton:checked {"
        "    background-color: #f44336;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
        "QPushButton:checked:hover {"
        "    background-color: #d32f2f;"
        "}"
        );

    // Create status label
    m_deviceStatusLabel = new QLabel("Device Status: OFF", this);
    m_deviceStatusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_deviceStatusLabel->setStyleSheet(
        "color: #FFFFFF; font-size: 16px; font-weight: bold; padding-left: 20px;"
        );

    // Add widgets to device control layout
    deviceControlLayout->addWidget(m_deviceToggleButton);
    deviceControlLayout->addWidget(m_deviceStatusLabel);
    deviceControlLayout->addStretch();

    // Add device control layout to main layout
    mainLayout->addLayout(deviceControlLayout);

    // Connect toggle button signal to slot
    connect(m_deviceToggleButton, &QPushButton::toggled, this, &LocationDetailDialog::toggleDevice);

    // Set size to fill most of the screen
    QScreen *screen = QApplication::primaryScreen();
    QSize screenSize = screen->size();
    resize(screenSize.width() * 0.9, screenSize.height() * 0.9);

    // Center on screen
    setGeometry(QStyle::alignedRect(
        Qt::LeftToRight,
        Qt::AlignCenter,
        size(),
        screen->geometry()
        ));
}


void LocationDetailDialog::toggleDevice(bool checked)
{
    m_deviceRunning = checked;

    if (m_deviceRunning) {
        m_deviceToggleButton->setText("Turn Device OFF");
        m_deviceStatusLabel->setText("Device Status: ON");
        sendDeviceCommand("MOTOR ON");
    } else {
        m_deviceToggleButton->setText("Turn Device ON");
        m_deviceStatusLabel->setText("Device Status: OFF");
        sendDeviceCommand("MOTOR OFF");
    }
}

// void LocationDetailDialog::sendDeviceCommand(const QString &command)
// {
//     if (m_socket && m_socket->isValid()) {
//         // Assuming deviceId is stored in the class or can be retrieved
//         // QString deviceId = this->m_id; // Use your actual device ID variable

//         // Create a JSON message
//         QJsonObject jsonMessage;
//         // jsonMessage["device_id"] = deviceId;
//         jsonMessage["command"] = command;
//         // jsonMessage["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

//         QJsonDocument doc(jsonMessage);
//         QString message = doc.toJson(QJsonDocument::Compact);

//         // Send the message through WebSocket
//         m_socket->sendTextMessage(message);

//         qDebug() << "Sent command to device:" << "-" << command;
//     } else {
//         qWarning() << "WebSocket is not connected. Cannot send command:" << command;

//         // Optional: Show message to user
//         QMessageBox::warning(this, "Connection Error",
//                              "Cannot send command - WebSocket is not connected.",
//                              QMessageBox::Ok);
//     }
// }

void LocationDetailDialog::sendDeviceCommand(const QString &command)
{
    if (m_socket && m_socket->isValid()) {
        // Send the plain text message directly
        m_socket->sendTextMessage(command);

        qDebug() << "Sent command to device:" << "-" << command;
    } else {
        qWarning() << "WebSocket is not connected. Cannot send command:" << command;

        // Optional: Show message to user
        QMessageBox::warning(this, "Connection Error",
                             "Cannot send command - WebSocket is not connected.",
                             QMessageBox::Ok);
    }
}


void LocationDetailDialog::handleSocketMessage(const QString &message) {  // 🛠 fixed spelling
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << parseError.errorString();
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

        qDebug() << "[Dialog JSON] Voltage:" << v1 <<" "<< v2 <<" "<< v3
                 << "Current:" << c1 <<" " << c2 << " " << c3
                 << "Power:" << p1 <<" " << p2 << " " << p3;


        m_gauges[0]->setValue(v1);
        m_gauges[1]->setValue(c1);
        m_gauges[2]->setValue(p1);
        m_gauges[3]->setValue(v2);
        m_gauges[4]->setValue(c2);
        m_gauges[5]->setValue(p2);
        m_gauges[6]->setValue(v3);
        m_gauges[7]->setValue(c3);
        m_gauges[8]->setValue(p3);


        // Update your UI here
    } else {
        qWarning() << "JSON missing expected keys";
    }
}

// void LocationDetailDialog::updateValues()
// {
//     for (auto gauge : m_gauges) {
//         // gauge->setRandomValue();
//     }
// }

void LocationDetailDialog::keyPressEvent(QKeyEvent *event)
{
    // Close dialog on Escape key
    if (event->key() == Qt::Key_Escape) {
        accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}
