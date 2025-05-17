#include "ScheduleManagerDialog.h"
#include "LocationDetailDialog.h"
#include <QFont>
#include <QMessageBox>
#include <QtWebSockets/QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>


ScheduleManagerDialog::ScheduleManagerDialog(QWidget* parent, int locationIndex, const QString& locationName, QColor locationColor,QString& topic,QWebSocket* socket)
    : QDialog(parent),
    m_locationIndex(locationIndex),
    m_locationName(locationName),
    m_locationColor(locationColor),
    m_topic(topic),
    m_socket(socket)


{
    manager = new QNetworkAccessManager(this);  //setup network MAnager
    setWindowTitle(QString("Schedule Manager - %1").arg(locationName));
    setMinimumSize(800, 600);
    setupUI();
    loadSchedules();
}

void ScheduleManagerDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);

    // Header
    QLabel* headerLabel = new QLabel(QString("Schedule Manager for %1").arg(m_locationName));
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(14);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    headerLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(headerLabel);

    // Schedule list
    scheduleListWidget = new QListWidget(this);
    scheduleListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    scheduleListWidget->setSpacing(8);  // 10 pixels between items

    mainLayout->addWidget(scheduleListWidget);

    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* addButton = new QPushButton("Add New Schedule");
    connect(addButton, &QPushButton::clicked, this, &ScheduleManagerDialog::addNewSchedule);

    QPushButton* deleteButton = new QPushButton("Delete Schedule");
    connect(deleteButton, &QPushButton::clicked, this, &ScheduleManagerDialog::deleteSchedule);

    QPushButton* closeButton = new QPushButton("Close");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}


void ScheduleManagerDialog::loadSchedules()
{
    // In a real app, you would load schedules from your data store
    // For example:
    // schedules = getSchedulesForLocation(m_locationIndex);
    // DO IT FROM DATABASE

    QJsonObject msg;
    msg["cluster_id"] = "1";
    msg["topic_name"] = m_topic;

    QJsonDocument doc(msg);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);


    //setup Request
    QNetworkRequest request(QUrl("http://localhost:8080/scheduler"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    //send GETRequest
    QNetworkReply* reply = manager->get(request,jsonData);

    QObject::connect(reply,&QNetworkReply::finished,this,[this,reply](){
        if(reply->error() == QNetworkReply::NoError){

            scheduleListWidget->clear();

            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);

            QJsonArray data = doc.array();

            for(int i = 0;i<data.size();i++){
                QJsonArray row = data[i].toArray();

                Schedule newSchedule;
                QString timeStr = row[0].toString();
                qDebug() << "timeSTr"<<timeStr;
                newSchedule.startTime = QTime::fromString(timeStr.left(8), "HH:mm:ss");
                timeStr = row[1].toString();
                newSchedule.endTime = QTime::fromString(timeStr.left(8), "HH:mm:ss");
                newSchedule.isActive = true;     /// WE HAVE TO DEVELOP LOGIC FOR THAT

                schedules.append(newSchedule);

                updateScheduleListItem(i);


            }



        }else{
            qDebug() << "Error" << reply->errorString();
        }
        reply->deleteLater();
    });

}

void ScheduleManagerDialog::updateScheduleListItem(int scheduleIndex)
{
    if (scheduleIndex < 0 || scheduleIndex >= schedules.size())
        return;

    const Schedule& schedule = schedules[scheduleIndex];
    qDebug() << schedule.startTime;
    // Create or update list item
    QString itemText = QString("%1 - %2")
                           .arg(schedule.startTime.toString("hh:mm AP"))
                           .arg(schedule.endTime.toString("hh:mm AP"));

    QListWidgetItem* item;
    if (scheduleIndex < scheduleListWidget->count()) {
        item = scheduleListWidget->item(scheduleIndex);
        item->setText(itemText);
        qDebug() << "first" ;
    } else {
        item = new QListWidgetItem(itemText);
        scheduleListWidget->addItem(item);
        qDebug() << "Second" ;
    }

    // Create widget for the item with toggle button
    QWidget* itemWidget = new QWidget();
    QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(5, 0, 5, 0);

    QLabel* timeLabel = new QLabel(itemText);
    itemLayout->addWidget(timeLabel);

    itemLayout->addStretch();

    QPushButton* toggleButton = new QPushButton(schedule.isActive ? "Active" : "Inactive");
    toggleButton->setStyleSheet(schedule.isActive ?
                                    "background-color: #27ae60; color: white;" :
                                    "background-color: #e74c3c; color: white;");

    connect(toggleButton, &QPushButton::clicked, this, [this, scheduleIndex, toggleButton]() {
        toggleScheduleActive(scheduleIndex, toggleButton);
    });

    itemLayout->addWidget(toggleButton);
    itemWidget->setLayout(itemLayout);

    scheduleListWidget->setItemWidget(item, itemWidget);
}


void ScheduleManagerDialog::toggleScheduleActive(int scheduleIndex, QPushButton* button)
{
    if (scheduleIndex < 0 || scheduleIndex >= schedules.size())
        return;

    schedules[scheduleIndex].isActive = !schedules[scheduleIndex].isActive;


    bool isActive = schedules[scheduleIndex].isActive;
    button->setText(isActive ? "Active" : "Inactive");
    button->setStyleSheet(isActive ?
                              "background-color: #27ae60; color: white;" :
                              "background-color: #e74c3c; color: white;");

    // In real app, you would save this change to your data store
    saveSchedules();
}

void ScheduleManagerDialog::addNewSchedule()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Schedule");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* startLabel = new QLabel("Start Time:");
    QTimeEdit* startTimeEdit = new QTimeEdit(QTime::currentTime());
    startTimeEdit->setDisplayFormat("hh:mm AP");

    QLabel* endLabel = new QLabel("End Time:");
    QTimeEdit* endTimeEdit = new QTimeEdit(QTime::currentTime().addSecs(3600)); // Add 1 hour by default
    endTimeEdit->setDisplayFormat("hh:mm AP");

    layout->addWidget(startLabel);
    layout->addWidget(startTimeEdit);
    layout->addWidget(endLabel);
    layout->addWidget(endTimeEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        // Validate times
        QTime startTime = startTimeEdit->time();
        QTime endTime = endTimeEdit->time();

        if (endTime <= startTime) {
            QMessageBox::warning(this, "Invalid Time Range",
                                 "End time must be after start time.");
            return;
        }

        Schedule newSchedule;
        newSchedule.startTime = startTime;
        newSchedule.endTime = endTime;
        newSchedule.isActive = true;

        schedules.append(newSchedule);
        qDebug() << "schedules size : " << schedules.size() ;
        updateScheduleListItem(schedules.size() - 1);

        saveScheduleToDB(startTime,endTime);
        saveSchedules();

    }
}

void ScheduleManagerDialog::deleteSchedule()
{
    int currentRow = scheduleListWidget->currentRow();
    if (currentRow >= 0 && currentRow < schedules.size()) {
        // Confirm deletion
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  "Delete Schedule",
                                                                  "Are you sure you want to delete this schedule?",
                                                                  QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            auto selectedSchedule = schedules.at(currentRow);
            schedules.remove(currentRow);
            delete scheduleListWidget->takeItem(currentRow);
            deleteScheduleFromDB(selectedSchedule.startTime,selectedSchedule.endTime);
            saveSchedules();
        }
    } else {
        QMessageBox::information(this, "No Selection",
                                 "Please select a schedule to delete.");
    }
}

void ScheduleManagerDialog::saveScheduleToDB(QTime startTime,QTime endTime){
    QJsonObject msg;
    msg["cluster_id"] = "1";
    msg["topic_name"] = m_topic;
    msg["scheduling_id"] = "1";
    msg["start_time"] = startTime.toString(Qt::ISODate);
    msg["end_time"] = endTime.toString(Qt::ISODate);

    QJsonDocument doc(msg);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);


    //setup Request
    QNetworkRequest request(QUrl("http://localhost:8080/scheduler"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    //send PostRequest
    QNetworkReply* reply = manager->post(request,jsonData);

    QObject::connect(reply,&QNetworkReply::finished,this,[reply](){
        if(reply->error() == QNetworkReply::NoError){
            QByteArray response = reply->readAll();
            qDebug() << "Response from BAckend: " << response;
        }else{
            qDebug() << "Error" << reply->errorString();
        }
        reply->deleteLater();
    });



}

void ScheduleManagerDialog::deleteScheduleFromDB(QTime startTime,QTime endTime){
    QJsonObject msg;
    msg["cluster_id"] = "1";
    msg["topic_name"] = m_topic;
    msg["scheduling_id"] = "1";
    msg["start_time"] = startTime.toString(Qt::ISODate);
    msg["end_time"] = endTime.toString(Qt::ISODate);

    QJsonDocument doc(msg);
    QByteArray jsonData = doc.toJson();



    //setup Request
    QNetworkRequest request(QUrl("http://localhost:8080/scheduler"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    //send PostRequest
    QNetworkReply* reply = manager->sendCustomRequest(request,"DELETE",jsonData);

    QObject::connect(reply,&QNetworkReply::finished,[reply](){
        if(reply->error() == QNetworkReply::NoError){
            QByteArray response = reply->readAll();
            qDebug() << "Response from BAckend: " << response;
        }else{
            qDebug() << "Error" << reply->errorString();
        }
        reply->deleteLater();
    });
}


void ScheduleManagerDialog::saveSchedules()
{
    // In a real app, you would save schedules to your data store
    // For example:
    // saveSchedulesToDatabase(m_locationIndex, schedules);

    // Notify that schedules have changed



    emit schedulesChanged(m_locationIndex, schedules);
}
