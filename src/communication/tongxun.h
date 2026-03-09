#ifndef TONGXUN_H
#define TONGXUN_H

#include <QObject>
#include <QString>
#include <QByteArray>

class TongXun : public QObject
{
    Q_OBJECT

public:
    explicit TongXun(QObject *parent = nullptr);
    virtual ~TongXun();

    // 核心通信接口
    virtual bool connect(const QString &address) = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const = 0;

    // 数据发送接口
    virtual bool sendData(const QByteArray &data) = 0;
    virtual bool sendCommand(const QString &command) = 0;

    // 状态查询接口
    virtual QString getConnectionStatus() const = 0;
    virtual QString getLastError() const = 0;

    // 配置接口
    virtual void settimeout(int milliseconds) = 0;
    virtual int gettimeout() const = 0;

signals:
    void connectionStatusChanged(bool connected);
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);
    void messageReceived(const QString &message);

public slots:
    virtual void onConnect() = 0;
    virtual void onDisconnect() = 0;
    virtual void onSendData(const QByteArray &data) = 0;
    virtual void onReceiveData(const QByteArray &data) = 0;
};

#endif // TONGXUN_H