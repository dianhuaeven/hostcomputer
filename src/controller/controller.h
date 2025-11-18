#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QString>
#include <QByteArray>

/**
 * @brief Controller层 - 业务逻辑处理核心
 *
 * 职责：
 * 1. 协调Model、Parser、Communication三层
 * 2. 处理用户交互逻辑
 * 3. 管理电机控制状态
 * 4. 提供统一的业务接口
 */
class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

    // 初始化和启动
    bool initialize();
    void start();
    void stop();

    // 通信接口
    bool connectCAN(const QString &interface);
    void disconnectCAN();
    bool isConnected() const;

signals:
    // 通信状态信号
    void connectionStatusChanged(bool connected);
    void dataReceived(const QByteArray &data);

    // 系统状态信号
    void systemError(const QString &error);
    void operationCompleted(const QString &operation);

private slots:
    // 内部处理槽函数
    void onDataReceived(const QByteArray &data);
    void onConnectionChanged(bool connected);

private:
    // 内部辅助方法
    void setupConnections();
    void processReceivedData(const QByteArray &data);
    void handleError(const QString &error);

private:
    // 状态变量
    bool m_initialized;
    bool m_running;
    bool m_connected;
};

#endif // CONTROLLER_H