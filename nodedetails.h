#ifndef NODEDETAILS_H
#define NODEDETAILS_H


class GaugeWidget : public QWidget {
    Q_OBJECT
public:
    explicit GaugeWidget(const QString& title, double min, double max,
                         const QColor& color, const QString& unit, QWidget* parent = nullptr);
    void setValue(double value);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title;
    QString m_unit;
    double m_min;
    double m_max;
    double m_value;
    QColor m_color;
    int m_decimals;
};


class LocationDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit LocationDetailDialog(const QString& locationName, QWidget* parent = nullptr);
    void updateStats(int voltage, int current, int power);
private:
    // QLabel* voltageLabel;
    // QLabel* currentLabel;
    // QLabel* powerLabel;
    // QProgressBar* voltageBar;
    // QProgressBar* currentBar;
    // QProgressBar* powerBar;
    GaugeWidget* voltageGauge;
    GaugeWidget* currentGauge;
    GaugeWidget* powerGauge;
    GaugeWidget* energyGauge;
    GaugeWidget* frequencyGauge;
    GaugeWidget* powerFactorGauge;

    // Data values storage
    double m_energy;
    QTimer* updateTimer;
    QString m_locationName;

private slots:
    void updateRandomValues();

};

#endif // NODEDETAILS_H
