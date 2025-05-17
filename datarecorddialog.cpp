#include "datarecorddialog.h"
#include <QFont>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollBar>
#include <QDateTime>
#include <QNetworkRequest>
#include <QUrl>
#include <QPdfWriter>
#include <QTextDocument>
#include <QDesktopServices>
#include <QFile>
#include <QTextStream>
#include <QWebSocket>

DataRecordDialog::DataRecordDialog(QWidget* parent, int locationIndex, const QString& locationName,
                                   QColor locationColor, QString& topic, QWebSocket* socket)
    : QDialog(parent),
    m_locationIndex(locationIndex),
    m_locationName(locationName),
    m_locationColor(locationColor),
    m_topic(topic),
    m_socket(socket)
{
    setWindowTitle(QString("Data Recording - %1").arg(locationName));
    setMinimumSize(1000, 600);
    networkManager = new QNetworkAccessManager(this);
    setupUI();
}

DataRecordDialog::~DataRecordDialog()
{
    // Clean up resources if needed
}

void DataRecordDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);

    // Header
    QLabel* headerLabel = new QLabel(QString("Data Recording for %1").arg(m_locationName));
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(14);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    headerLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(headerLabel);

    // Date/Time selection
    QHBoxLayout* dateTimeLayout = new QHBoxLayout();

    QLabel* startLabel = new QLabel("Start Date/Time:");
    startDateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1));
    startDateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    startDateTimeEdit->setCalendarPopup(true);

    QLabel* endLabel = new QLabel("End Date/Time:");
    endDateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    endDateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    endDateTimeEdit->setCalendarPopup(true);

    fetchButton = new QPushButton("Fetch Data");
    connect(fetchButton, &QPushButton::clicked, this, &DataRecordDialog::fetchData);

    dateTimeLayout->addWidget(startLabel);
    dateTimeLayout->addWidget(startDateTimeEdit);
    dateTimeLayout->addWidget(endLabel);
    dateTimeLayout->addWidget(endDateTimeEdit);
    dateTimeLayout->addWidget(fetchButton);

    mainLayout->addLayout(dateTimeLayout);

    // Table for data display
    dataTable = new QTableWidget(this);
    dataTable->setColumnCount(13); // Updated column count for all values
    dataTable->setHorizontalHeaderLabels(QStringList() 
        << "Timestamp" 
        << "Total Power" 
        << "P1" << "P2" << "P3"
        << "Total Current" 
        << "C1" << "C2" << "C3"
        << "Total Voltage" 
        << "V1" << "V2" << "V3");
    dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Read-only
    dataTable->setAlternatingRowColors(true);
    mainLayout->addWidget(dataTable);

    // Export buttons
    QHBoxLayout* exportLayout = new QHBoxLayout();

    exportPDFButton = new QPushButton("Export as PDF");
    connect(exportPDFButton, &QPushButton::clicked, this, &DataRecordDialog::generatePDF);
    exportPDFButton->setEnabled(false); // Enable only after data is fetched

    exportCSVButton = new QPushButton("Export as CSV");
    connect(exportCSVButton, &QPushButton::clicked, this, &DataRecordDialog::exportCSV);
    exportCSVButton->setEnabled(false); // Enable only after data is fetched

    QPushButton* closeButton = new QPushButton("Close");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    exportLayout->addWidget(exportPDFButton);
    exportLayout->addWidget(exportCSVButton);
    exportLayout->addStretch();
    exportLayout->addWidget(closeButton);

    mainLayout->addLayout(exportLayout);
}

void DataRecordDialog::fetchData()
{
    // Prepare JSON request
    qDebug() << "FEtch data called";
    QJsonObject requestObj;
    requestObj["cluster_id"] = "1";   // HAVE TO WORK OUT
    requestObj["topic_name"] = m_topic;
    requestObj["start_time"] = startDateTimeEdit->dateTime().toString(Qt::ISODate);
    requestObj["end_time"] = endDateTimeEdit->dateTime().toString(Qt::ISODate);

    QJsonDocument requestDoc(requestObj);
    QByteArray jsonData = requestDoc.toJson(QJsonDocument::Compact);

    //  FOR HTTP REQUEST

    //setup Request
    QNetworkRequest request(QUrl("http://localhost:8080/recordData"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    //send GET_Request
    QNetworkReply* reply = networkManager->get(request,jsonData);

    QObject::connect(reply,&QNetworkReply::finished,this,[this,reply](){
        if(reply->error() == QNetworkReply::NoError){

            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);

            if (doc.isArray()) {
                QJsonArray dataArray = doc.array();
                populateTableWithData(dataArray);

                // Enable export buttons now that we have data
                exportPDFButton->setEnabled(true);
                exportCSVButton->setEnabled(true);

                // Scroll to top of table
                dataTable->verticalScrollBar()->setValue(0);
            } else {
                QMessageBox::warning(this, "Error", "Invalid response format from server");
            }

        }else{
            qDebug() << "Error" << reply->errorString();
        }
        reply->deleteLater();

    });



    // Disable the fetch button while request is in progress
    fetchButton->setEnabled(false);
    fetchButton->setText("Fetching...");
}

void DataRecordDialog::populateTableWithData(const QJsonArray& data)
{
    // Clear existing table data
    dataTable->setRowCount(0);

    // Add rows for each data item
    for (int i = 0; i < data.size(); ++i) {
        QJsonObject item = data[i].toObject();

        // Add a new row
        int row = dataTable->rowCount();
        dataTable->insertRow(row);

        // Timestamp
        dataTable->setItem(row, 0, new QTableWidgetItem(item["timestamp"].toString()));

        // Calculate total power
        double p1 = item["p1"].toDouble();
        double p2 = item["p2"].toDouble();
        double p3 = item["p3"].toDouble();
        double totalPower = p1 + p2 + p3;

        // Power values
        dataTable->setItem(row, 1, new QTableWidgetItem(QString::number(totalPower, 'f', 2)));
        dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(p1, 'f', 2)));
        dataTable->setItem(row, 3, new QTableWidgetItem(QString::number(p2, 'f', 2)));
        dataTable->setItem(row, 4, new QTableWidgetItem(QString::number(p3, 'f', 2)));

        // Calculate total current
        double c1 = item["c1"].toDouble();
        double c2 = item["c2"].toDouble();
        double c3 = item["c3"].toDouble();
        double totalCurrent = c1 + c2 + c3;

        // Current values
        dataTable->setItem(row, 5, new QTableWidgetItem(QString::number(totalCurrent, 'f', 2)));
        dataTable->setItem(row, 6, new QTableWidgetItem(QString::number(c1, 'f', 2)));
        dataTable->setItem(row, 7, new QTableWidgetItem(QString::number(c2, 'f', 2)));
        dataTable->setItem(row, 8, new QTableWidgetItem(QString::number(c3, 'f', 2)));

        // Calculate total voltage
        double v1 = item["v1"].toDouble();
        double v2 = item["v2"].toDouble();
        double v3 = item["v3"].toDouble();
        double totalVoltage = (v1 + v2 + v3) / 3.0; // Average voltage

        // Voltage values
        dataTable->setItem(row, 9, new QTableWidgetItem(QString::number(totalVoltage, 'f', 2)));
        dataTable->setItem(row, 10, new QTableWidgetItem(QString::number(v1, 'f', 2)));
        dataTable->setItem(row, 11, new QTableWidgetItem(QString::number(v2, 'f', 2)));
        dataTable->setItem(row, 12, new QTableWidgetItem(QString::number(v3, 'f', 2)));
    }

    QMessageBox::information(this, "Data Loaded",
                             QString("Successfully loaded %1 records").arg(dataTable->rowCount()));
}

void DataRecordDialog::generatePDF()
{
    if (dataTable->rowCount() == 0) {
        QMessageBox::warning(this, "No Data", "There is no data to export.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save PDF File",
                                                    QString("%1_Data_%2.pdf").arg(m_locationName)
                                                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                    "PDF Files (*.pdf)");
    if (fileName.isEmpty())
        return;

    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setPageMargins(QMarginsF(30, 30, 30, 30));

    QPainter painter(&pdfWriter);
    painter.setPen(Qt::black);

    // Draw header
    QFont titleFont = painter.font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    painter.setFont(titleFont);

    QRect titleRect(0, 100, pdfWriter.width(), 50);
    painter.drawText(titleRect, Qt::AlignHCenter, QString("Data Report - %1").arg(m_locationName));

    // Draw date range
    QFont normalFont = painter.font();
    normalFont.setPointSize(10);
    normalFont.setBold(false);
    painter.setFont(normalFont);

    QRect dateRect(0, 200, pdfWriter.width(), 30);
    painter.drawText(dateRect, Qt::AlignHCenter,
                     QString("Period: %1 to %2")
                         .arg(startDateTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm"))
                         .arg(endDateTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm")));

    // Table header
    QFont tableHeaderFont = painter.font();
    tableHeaderFont.setBold(true);
    painter.setFont(tableHeaderFont);

    int yPos = 300;
    int rowHeight = 40;
    int colWidth = pdfWriter.width() / 10;

    // Draw table headers
    for (int col = 0; col < dataTable->columnCount(); ++col) {
        QRect headerRect(col * colWidth, yPos, colWidth, rowHeight);
        painter.drawRect(headerRect);
        painter.drawText(headerRect, Qt::AlignCenter, dataTable->horizontalHeaderItem(col)->text());
    }

    // Draw table data
    painter.setFont(normalFont);
    int maxRowsPerPage = (pdfWriter.height() - yPos - 100) / rowHeight;
    int currentPage = 1;

    for (int row = 0; row < dataTable->rowCount(); ++row) {
        // Check if we need a new page
        if (row > 0 && row % maxRowsPerPage == 0) {
            painter.end();
            pdfWriter.newPage();
            painter.begin(&pdfWriter);
            painter.setPen(Qt::black);
            painter.setFont(normalFont);

            // Reset yPos for new page and draw header
            yPos = 100;
            currentPage++;

            // Draw page header on new page
            painter.setFont(tableHeaderFont);
            for (int col = 0; col < dataTable->columnCount(); ++col) {
                QRect headerRect(col * colWidth, yPos, colWidth, rowHeight);
                painter.drawRect(headerRect);
                painter.drawText(headerRect, Qt::AlignCenter, dataTable->horizontalHeaderItem(col)->text());
            }
            painter.setFont(normalFont);
            yPos += rowHeight;
        } else if (row > 0) {
            yPos += rowHeight;
        } else {
            yPos += rowHeight;
        }

        // Draw row data
        for (int col = 0; col < dataTable->columnCount(); ++col) {
            QTableWidgetItem* item = dataTable->item(row, col);
            QRect cellRect(col * colWidth, yPos, colWidth, rowHeight);
            painter.drawRect(cellRect);
            if (item) {
                painter.drawText(cellRect, Qt::AlignCenter, item->text());
            }
        }
    }

    painter.end();

    QMessageBox::information(this, "PDF Created",
                             QString("PDF file has been saved to:\n%1").arg(fileName));

    // Open the PDF file
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

void DataRecordDialog::handleNetworkReply(QNetworkReply* reply)
{
    // Check for errors in the network reply
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "Network Error",
                             QString("Error in network request: %1").arg(reply->errorString()));
        fetchButton->setEnabled(true);
        fetchButton->setText("Fetch Data");
        reply->deleteLater();
        return;
    }

    // Read the response data
    QByteArray responseData = reply->readAll();

    // Parse JSON response
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isArray()) {
        QMessageBox::warning(this, "Error", "Invalid response format from server");
        fetchButton->setEnabled(true);
        fetchButton->setText("Fetch Data");
        reply->deleteLater();
        return;
    }

    // Process the data
    QJsonArray dataArray = doc.array();
    populateTableWithData(dataArray);

    // Enable export buttons now that we have data
    exportPDFButton->setEnabled(true);
    exportCSVButton->setEnabled(true);

    // Scroll to top of table
    dataTable->verticalScrollBar()->setValue(0);

    // Re-enable the fetch button
    fetchButton->setEnabled(true);
    fetchButton->setText("Fetch Data");

    // Clean up
    reply->deleteLater();
}

void DataRecordDialog::exportCSV()
{
    if (dataTable->rowCount() == 0) {
        QMessageBox::warning(this, "No Data", "There is no data to export.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save CSV File",
                                                    QString("%1_Data_%2.csv").arg(m_locationName)
                                                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                    "CSV Files (*.csv)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);

    // Write header
    QStringList headers;
    for (int col = 0; col < dataTable->columnCount(); ++col) {
        headers << dataTable->horizontalHeaderItem(col)->text();
    }
    out << headers.join(",") << "\n";

    // Write data
    for (int row = 0; row < dataTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < dataTable->columnCount(); ++col) {
            QTableWidgetItem* item = dataTable->item(row, col);
            if (item) {
                // Quote data if it contains commas
                QString text = item->text();
                if (text.contains(",")) {
                    rowData << "\"" + text + "\"";
                } else {
                    rowData << text;
                }
            } else {
                rowData << "";
            }
        }
        out << rowData.join(",") << "\n";
    }

    file.close();

    QMessageBox::information(this, "CSV Created",
                             QString("CSV file has been saved to:\n%1").arg(fileName));

    // Open the CSV file
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

