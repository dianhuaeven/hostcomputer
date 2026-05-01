#include "ControlPanelWidget.h"

#include "GamepadDisplayWidget.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ControlPanelWidget::ControlPanelWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    auto *statusGrid = new QGridLayout();
    statusGrid->setHorizontalSpacing(8);
    statusGrid->setVerticalSpacing(4);

    auto *modeLabel = new QLabel("模式", this);
    m_modeValue = new QLabel("车体运动", this);
    auto *gamepadLabel = new QLabel("手柄", this);
    m_gamepadValue = new QLabel("未连接", this);

    statusGrid->addWidget(modeLabel, 0, 0);
    statusGrid->addWidget(m_modeValue, 0, 1);
    statusGrid->addWidget(gamepadLabel, 1, 0);
    statusGrid->addWidget(m_gamepadValue, 1, 1);

    auto *buttonLayout = new QHBoxLayout();
    m_gamepadButton = new QPushButton("连接手柄", this);
    m_emergencyButton = new QPushButton("急停", this);
    m_emergencyButton->setObjectName("controlPanelEmergencyButton");
    buttonLayout->addWidget(m_gamepadButton);
    buttonLayout->addWidget(m_emergencyButton);

    m_gamepadDisplay = new GamepadDisplayWidget(this);

    mainLayout->addLayout(statusGrid);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_gamepadDisplay, 1);

    connect(m_gamepadButton, &QPushButton::clicked, this, &ControlPanelWidget::gamepadConnectRequested);
    connect(m_emergencyButton, &QPushButton::clicked, this, &ControlPanelWidget::emergencyStopRequested);

    setStyleSheet(
        "ControlPanelWidget { background: #ffffff; border: 1px solid #dbe3ef; border-radius: 6px; }"
        "ControlPanelWidget QLabel { color: #111827; font-size: 13px; }"
        "ControlPanelWidget QPushButton { min-height: 28px; padding: 4px 10px; }"
        "QPushButton#controlPanelEmergencyButton { background: #b91c1c; color: white; font-weight: 700; }"
        "QPushButton#controlPanelEmergencyButton:hover { background: #991b1b; }");
}

void ControlPanelWidget::setModeText(const QString &modeText)
{
    m_modeValue->setText(modeText);
}

void ControlPanelWidget::setGamepadConnected(bool connected)
{
    m_gamepadValue->setText(connected ? "已连接" : "未连接");
    m_gamepadValue->setStyleSheet(connected ? "color: #15803d; font-weight: 600;" : "color: #b91c1c; font-weight: 600;");
    m_gamepadButton->setText(connected ? "断开手柄" : "连接手柄");
}

void ControlPanelWidget::updateGamepadAxes(float lx, float ly, float rx, float ry, float lt, float rt)
{
    m_gamepadDisplay->updateAll(lx, ly, rx, ry, lt, rt);
}

void ControlPanelWidget::updateGamepadButton(const QString &buttonName, bool pressed)
{
    m_gamepadDisplay->updateButton(buttonName, pressed);
}
