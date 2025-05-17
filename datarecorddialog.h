#ifndef DATARECORDDIALOG_H
#define DATARECORDDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QTableWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtWebSockets/QWebSocket>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QFileDialog>

class DataRecordDialog : public QDialog
{
    Q_OBJECT

public:
    DataRecordDialog(QWidget* parent, int locationIndex, const QString& locationName,
                     QColor locationColor, QString& topic, QWebSocket* socket);
    ~DataRecordDialog();

private slots:
    void fetchData();
    void handleNetworkReply(QNetworkReply* reply);
    void generatePDF();
    void exportCSV();

private:
    void setupUI();
    void populateTableWithData(const QJsonArray& data);

    QVBoxLayout* mainLayout;
    QTableWidget* dataTable;
    QDateTimeEdit* startDateTimeEdit;
    QDateTimeEdit* endDateTimeEdit;
    QPushButton* fetchButton;
    QPushButton* exportPDFButton;
    QPushButton* exportCSVButton;
    QNetworkAccessManager* networkManager;

    int m_locationIndex;
    QString m_locationName;
    QColor m_locationColor;
    QString m_topic;
    QWebSocket* m_socket;
};

#endif // DATARECORDDIALOG_H
