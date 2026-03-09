#ifndef KEYBOARDCONTROLLER_H
#define KEYBOARDCONTROLLER_H

#include <QObject>
#include <QSet>
#include <QTimer>
#include <QKeyEvent>

/**
 * @brief 键盘输入驱动 - 将键盘按键映射为小车运动命令
 *
 * 键位映射:
 *   W/↑  - 前进
 *   S/↓  - 后退
 *   A/←  - 左转
 *   D/→  - 右转
 *   Space - 急停
 *
 * 支持多键同时按下(如 W+A = 前进+左转)
 * 松开所有键后自动发送零速度(停止)
 */
class KeyboardController : public QObject
{
    Q_OBJECT

public:
    explicit KeyboardController(QObject *parent = nullptr);
    ~KeyboardController();

    void handleKeyPress(QKeyEvent *event);
    void handleKeyRelease(QKeyEvent *event);

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setLinearSpeed(float speed);
    float linearSpeed() const;

    void setAngularSpeed(float speed);
    float angularSpeed() const;

signals:
    /// 速度命令信号: linearX=前后, angularZ=左右转
    void velocityChanged(float linearX, float linearY, float angularZ);
    void emergencyStopRequested();
    void enabledChanged(bool enabled);

private slots:
    void updateVelocity();

private:
    void computeVelocity();

    QSet<int> m_pressedKeys;
    QTimer *m_sendTimer;

    bool m_enabled;
    float m_linearSpeed;   // 基础线速度 m/s
    float m_angularSpeed;  // 基础角速度 rad/s

    float m_linearX;       // 前进/后退
    float m_angularZ;      // 左转/右转

    static const int SEND_INTERVAL_MS = 100;  // 10Hz
};

#endif // KEYBOARDCONTROLLER_H
