#ifndef CONTROLPANELWIDGET_H
#define CONTROLPANELWIDGET_H

#include <QString>
#include <QWidget>

class GamepadDisplayWidget;
class QLabel;
class QPushButton;

class ControlPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanelWidget(QWidget *parent = nullptr);

    void setModeText(const QString &modeText);
    void setGamepadConnected(bool connected);
    void updateGamepadAxes(float lx, float ly, float rx, float ry, float lt, float rt);
    void updateGamepadButton(const QString &buttonName, bool pressed);

signals:
    void emergencyStopRequested();
    void gamepadConnectRequested();

private:
    QLabel *m_modeValue = nullptr;
    QLabel *m_gamepadValue = nullptr;
    QPushButton *m_gamepadButton = nullptr;
    QPushButton *m_emergencyButton = nullptr;
    GamepadDisplayWidget *m_gamepadDisplay = nullptr;
};

#endif // CONTROLPANELWIDGET_H
