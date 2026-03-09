#include "KeyboardController.h"
#include <QDebug>

KeyboardController::KeyboardController(QObject *parent)
    : QObject(parent)
    , m_enabled(false)
    , m_linearSpeed(0.5f)
    , m_angularSpeed(1.0f)
    , m_linearX(0.0f)
    , m_angularZ(0.0f)
{
    m_sendTimer = new QTimer(this);
    m_sendTimer->setInterval(SEND_INTERVAL_MS);
    connect(m_sendTimer, &QTimer::timeout, this, &KeyboardController::updateVelocity);
}

KeyboardController::~KeyboardController()
{
    m_sendTimer->stop();
}

void KeyboardController::handleKeyPress(QKeyEvent *event)
{
    if (!m_enabled || event->isAutoRepeat()) return;

    int key = event->key();

    // 急停
    if (key == Qt::Key_Space) {
        m_pressedKeys.clear();
        m_linearX = 0.0f;
        m_angularZ = 0.0f;
        emit velocityChanged(0.0f, 0.0f, 0.0f);
        emit emergencyStopRequested();
        return;
    }

    m_pressedKeys.insert(key);
    computeVelocity();
}

void KeyboardController::handleKeyRelease(QKeyEvent *event)
{
    if (!m_enabled || event->isAutoRepeat()) return;

    m_pressedKeys.remove(event->key());
    computeVelocity();
}

void KeyboardController::computeVelocity()
{
    float lx = 0.0f;
    float az = 0.0f;

    // 前进 W / ↑
    if (m_pressedKeys.contains(Qt::Key_W) || m_pressedKeys.contains(Qt::Key_Up))
        lx += m_linearSpeed;

    // 后退 S / ↓
    if (m_pressedKeys.contains(Qt::Key_S) || m_pressedKeys.contains(Qt::Key_Down))
        lx -= m_linearSpeed;

    // 左转 A / ←
    if (m_pressedKeys.contains(Qt::Key_A) || m_pressedKeys.contains(Qt::Key_Left))
        az += m_angularSpeed;

    // 右转 D / →
    if (m_pressedKeys.contains(Qt::Key_D) || m_pressedKeys.contains(Qt::Key_Right))
        az -= m_angularSpeed;

    m_linearX = lx;
    m_angularZ = az;
}

void KeyboardController::updateVelocity()
{
    emit velocityChanged(m_linearX, 0.0f, m_angularZ);
}

void KeyboardController::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;

    if (enabled) {
        m_sendTimer->start();
    } else {
        m_sendTimer->stop();
        m_pressedKeys.clear();
        m_linearX = 0.0f;
        m_angularZ = 0.0f;
        // 禁用时发送一次零速度确保停车
        emit velocityChanged(0.0f, 0.0f, 0.0f);
    }

    emit enabledChanged(enabled);
}

bool KeyboardController::isEnabled() const { return m_enabled; }

void KeyboardController::setLinearSpeed(float speed) { m_linearSpeed = speed; }
float KeyboardController::linearSpeed() const { return m_linearSpeed; }

void KeyboardController::setAngularSpeed(float speed) { m_angularSpeed = speed; }
float KeyboardController::angularSpeed() const { return m_angularSpeed; }
