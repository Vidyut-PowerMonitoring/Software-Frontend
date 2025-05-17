// ModernGaugeWidget.cpp
#include "ModernGaugeWidget.h"
#include <QPainter>
#include <QtMath>
#include <QRandomGenerator>
#include <QtWebSockets/QWebSocket>

ModernGaugeWidget::ModernGaugeWidget(const QString &title, const QString &label, double minValue, double maxValue,
                                     const QString &units, const QColor &color, QWidget *parent)
    : QFrame(parent),
    m_title(title),
    m_label(label),
    m_minValue(minValue),
    m_maxValue(maxValue),
    m_units(units),
    m_value(minValue),
    m_color(color),
    m_targetValue(minValue),
    m_animationStep(0)
{
    // Dark theme
    setStyleSheet("background-color: #0F0F0F; color: #CCCCCC;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(180, 180);

    // Animation timer
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ModernGaugeWidget::updateAnimation);
    m_timer->start(30);
}

void ModernGaugeWidget::setValue(double value)
{
    m_targetValue = qBound(m_minValue, value, m_maxValue);
    m_animationStep = (m_targetValue - m_value) / 20.0;
}

// void ModernGaugeWidget::setRandomValue()
// {
//     // Generate a random value in the range
//     double range = m_maxValue - m_minValue;
//     double randomValue = m_minValue + (QRandomGenerator::global()->generateDouble() * range);
//     setValue(randomValue);
// }

QString ModernGaugeWidget::getLabelText() const
{
    // Map value to label (Low, Medium, High)
    double normalized = (m_value - m_minValue) / (m_maxValue - m_minValue);

    if (normalized < 0.3) {
        return "Low";
    } else if (normalized < 0.7) {
        return "Medium";
    } else {
        return "High";
    }
}

void ModernGaugeWidget::updateAnimation()
{
    if (qAbs(m_value - m_targetValue) > qAbs(m_animationStep * 0.5)) {
        m_value += m_animationStep;
        update();
    } else {
        m_value = m_targetValue;
    }
}

void ModernGaugeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int width = this->width();
    int height = this->height();
    int centerX = width / 2;
    int centerY = height / 2;
    int radius = qMin(width, height) / 2 - 10;

    // Draw black background
    painter.fillRect(rect(), QColor("#0F0F0F"));

    // Draw arc background (dark gray)
    painter.setPen(QPen(QColor("#333333"), 8, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(centerX - radius, centerY - radius, radius * 2, radius * 2, 225 * 16, 270 * 16);

    // Draw value arc
    double normalized = (m_value - m_minValue) / (m_maxValue - m_minValue);
    painter.setPen(QPen(m_color, 8, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(centerX - radius, centerY - radius, radius * 2, radius * 2, 225 * 16, normalized * 270 * 16);

    // Draw tick marks
    painter.setPen(QPen(QColor("#666666"), 1));
    for (int i = 0; i <= 27; i++) {
        double angle = 225 + i * 10;
        double radians = angle * M_PI / 180.0;
        int innerRadius = (i % 3 == 0) ? radius - 12 : radius - 8;
        int outerRadius = radius;

        int x1 = centerX + innerRadius * qCos(radians);
        int y1 = centerY + innerRadius * qSin(radians);
        int x2 = centerX + outerRadius * qCos(radians);
        int y2 = centerY + outerRadius * qSin(radians);

        painter.drawLine(x1, y1, x2, y2);
    }

    // Draw value text
    QFont valueFont("Arial", radius/3, QFont::Bold);
    painter.setFont(valueFont);
    painter.setPen(m_color);
    QString valueText = QString::number(m_value, 'f', 1);
    QRectF valueRect(0, centerY - radius/3, width, radius/2);
    painter.drawText(valueRect, Qt::AlignCenter, valueText);

    // Draw label text (Low, Medium, High)
    QFont labelFont("Arial", radius/5);
    painter.setFont(labelFont);
    painter.setPen(m_color);
    QRectF labelRect(0, centerY + radius/6, width, radius/3);
    painter.drawText(labelRect, Qt::AlignCenter, getLabelText());

    // Draw title text
    QFont titleFont("Arial", radius/6);
    painter.setFont(titleFont);
    painter.setPen(QColor("#999999"));
    QRectF titleRect(0, centerY + radius/2, width, radius/3);
    painter.drawText(titleRect, Qt::AlignCenter, m_title);
}
