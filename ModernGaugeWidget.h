// ModernGaugeWidget.h
#ifndef MODERNGAUGEWIDGET_H
#define MODERNGAUGEWIDGET_H

#include <QFrame>
#include <QTimer>
#include <QColor>

class ModernGaugeWidget : public QFrame
{
    Q_OBJECT

public:
    ModernGaugeWidget(const QString &title, const QString &label, double minValue, double maxValue,
                      const QString &units, const QColor &color, QWidget *parent = nullptr);
    void setValue(double value);
    void setRandomValue();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateAnimation();

private:
    QString m_title;
    QString m_label;
    double m_minValue;
    double m_maxValue;
    QString m_units;
    double m_value;
    QColor m_color;
    double m_targetValue;
    double m_animationStep;
    QTimer *m_timer;

    QString getLabelText() const;
};

#endif // MODERNGAUGEWIDGET_H
