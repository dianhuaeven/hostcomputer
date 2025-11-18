#include "controller.h"
#include <QDebug>

Controller::Controller(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_running(false)
    , m_connected(false)
{
}

Controller::~Controller()
{
    stop();
}

bool Controller::initialize()
{
    if (m_initialized) {
        return true;
    }

    // TODO: 初始化Parser和CANManager
    // m_parser = std::make_unique<Parser>();
    // m_canManager = std::make_unique<CANManager>();

    // 设置信号连接
    setupConnections();

    m_initialized = true;
    qDebug() << "Controller initialized";
    return true;
}

void Controller::start()
{
    if (!m_initialized) {
        qWarning() << "Controller not initialized";
        return;
    }

    m_running = true;
    qDebug() << "Controller started";
}

void Controller::stop()
{
    m_running = false;

    if (m_connected) {
        disconnectCAN();
    }

    qDebug() << "Controller stopped";
}

bool Controller::connectCAN(const QString &interface)
{
    // TODO: 实现CAN连接
    Q_UNUSED(interface)

    // 临时模拟连接成功
    m_connected = true;
    emit connectionStatusChanged(true);
    qDebug() << "CAN connected to:" << interface;

    return true;
}

void Controller::disconnectCAN()
{
    if (m_connected) {
        // TODO: 实现CAN断开连接
        m_connected = false;
        emit connectionStatusChanged(false);
        qDebug() << "CAN disconnected";
    }
}

bool Controller::isConnected() const
{
    return m_connected;
}

void Controller::setupConnections()
{
    // TODO: 设置Parser和CANManager的信号连接
    qDebug() << "Setting up connections";
}

void Controller::onDataReceived(const QByteArray &data)
{
    qDebug() << "Data received:" << data.toHex();
    emit dataReceived(data);
    processReceivedData(data);
}

void Controller::onConnectionChanged(bool connected)
{
    m_connected = connected;
    emit connectionStatusChanged(connected);
    qDebug() << "Connection status changed:" << connected;
}

void Controller::processReceivedData(const QByteArray &data)
{
    // TODO: 使用parser解析数据
    qDebug() << "Processing received data:" << data.toHex();
}

void Controller::handleError(const QString &error)
{
    qWarning() << "Controller error:" << error;
    emit systemError(error);
}