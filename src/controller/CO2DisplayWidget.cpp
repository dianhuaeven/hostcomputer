#include "CO2DisplayWidget.h"

CO2DisplayWidget::CO2DisplayWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    m_titleLabel = new QLabel("CO\u2082 \u6D53\u5EA6", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333;");

    m_valueLabel = new QLabel("-- ppm", this);
    m_valueLabel->setAlignment(Qt::AlignCenter);
    m_valueLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #2196F3;");

    m_statusLabel = new QLabel("\u7B49\u5F85\u6570\u636E...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 13px; color: #999;");

    layout->addStretch();
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_valueLabel);
    layout->addWidget(m_statusLabel);
    layout->addStretch();

    setStyleSheet("background-color: #f5f5f5; border: 1px solid #ddd; border-radius: 6px;");
}

void CO2DisplayWidget::setCO2Value(float ppm)
{
    m_valueLabel->setText(QString("%1 ppm").arg(ppm, 0, 'f', 0));
    updateStatusColor(ppm);
}

void CO2DisplayWidget::updateStatusColor(float ppm)
{
    QString valueColor;
    QString statusText;

    if (ppm < 800) {
        valueColor = "#4CAF50";  // 绿色
        statusText = "\u7A7A\u6C14\u826F\u597D";
    } else if (ppm < 1500) {
        valueColor = "#FF9800";  // 橙色
        statusText = "\u6CE8\u610F\u901A\u98CE";
    } else {
        valueColor = "#F44336";  // 红色
        statusText = "\u6D53\u5EA6\u8FC7\u9AD8!";
    }

    m_valueLabel->setStyleSheet(QString("font-size: 36px; font-weight: bold; color: %1;").arg(valueColor));
    m_statusLabel->setText(statusText);
    m_statusLabel->setStyleSheet(QString("font-size: 13px; font-weight: bold; color: %1;").arg(valueColor));
}
