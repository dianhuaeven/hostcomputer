#include "TelemetryPanelWidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QStringList>
#include <QVBoxLayout>

TelemetryPanelWidget::TelemetryPanelWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(8, 8, 8, 8);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);

    const QStringList keys = {
        "connection", "heartbeat", "fps", "bandwidth",
        "mode", "gamepad", "errors", "co2"
    };
    const QStringList titles = {
        "连接", "心跳", "FPS", "带宽",
        "模式", "手柄", "错误", "CO2"
    };
    const QStringList values = {
        "断开", "丢失", "0 FPS", "N/A",
        "车体运动", "未连接", "0", "-- ppm"
    };

    for (int i = 0; i < keys.size(); ++i) {
        QWidget *metric = new QWidget(this);
        auto *metricLayout = new QVBoxLayout(metric);
        metricLayout->setContentsMargins(10, 6, 10, 6);
        metricLayout->setSpacing(2);

        auto *titleLabel = new QLabel(titles[i], metric);
        titleLabel->setStyleSheet("color: #6b7280; font-size: 12px;");

        auto *valueLabel = new QLabel(values[i], metric);
        valueLabel->setStyleSheet("color: #111827; font-size: 15px; font-weight: 600;");
        valueLabel->setWordWrap(true);

        metricLayout->addWidget(titleLabel);
        metricLayout->addWidget(valueLabel);
        metric->setStyleSheet("background: #f8fafc; border: 1px solid #dbe3ef; border-radius: 6px;");

        m_valueLabels.insert(keys[i], valueLabel);
        grid->addWidget(metric, i / 2, i % 2);
    }

    setStyleSheet("TelemetryPanelWidget { background: #ffffff; }");
}

void TelemetryPanelWidget::setConnectionStatus(bool connected)
{
    setMetricValue("connection", connected ? "已连接" : "断开", connected ? "#15803d" : "#b91c1c");
}

void TelemetryPanelWidget::setHeartbeatStatus(bool online)
{
    setMetricValue("heartbeat", online ? "在线" : "丢失", online ? "#15803d" : "#b91c1c");
}

void TelemetryPanelWidget::setFps(int fps)
{
    setMetricValue("fps", QString("%1 FPS").arg(fps));
}

void TelemetryPanelWidget::setBandwidthText(const QString &text)
{
    setMetricValue("bandwidth", text);
}

void TelemetryPanelWidget::setModeText(const QString &text)
{
    setMetricValue("mode", text);
}

void TelemetryPanelWidget::setGamepadConnected(bool connected)
{
    setMetricValue("gamepad", connected ? "已连接" : "未连接", connected ? "#15803d" : "#b91c1c");
}

void TelemetryPanelWidget::setErrorCount(int count)
{
    setMetricValue("errors", QString::number(count), count > 0 ? "#b91c1c" : "#111827");
}

void TelemetryPanelWidget::setCO2Value(float ppm)
{
    QString color = "#15803d";
    if (ppm >= 1500.0f) {
        color = "#b91c1c";
    } else if (ppm >= 800.0f) {
        color = "#c2410c";
    }

    setMetricValue("co2", QString("%1 ppm").arg(ppm, 0, 'f', 0), color);
}

void TelemetryPanelWidget::setMetricValue(const QString &key, const QString &value, const QString &color)
{
    QLabel *label = m_valueLabels.value(key, nullptr);
    if (!label) {
        return;
    }

    label->setText(value);
    const QString effectiveColor = color.isEmpty() ? "#111827" : color;
    label->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 600;").arg(effectiveColor));
}
