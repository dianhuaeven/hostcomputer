#ifndef SERIALPORTMANAGER_H
#define SERIALPORTMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QTimer>
#include "../parser/ProtocolParser.h"
#include "SharedStructs.h"

namespace Communication {

/**
 * @brief 串口管理器类
 *
 * 负责串口设备的打开、关闭、数据收发等操作
 * 使用Qt信号槽机制实现异步通信
 */
class SerialPortManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortManager(QObject *parent = nullptr);
    ~SerialPortManager();

    // 串口设备管理
    QStringList getAvailablePorts() const;
    bool openPort(const QString &portName, int baudRate = 115200);
    void closePort();
    bool isOpen() const;

    // 串口配置
    bool setBaudRate(int baudRate);
    bool setDataBits(QSerialPort::DataBits dataBits);
    bool setParity(QSerialPort::Parity parity);
    bool setStopBits(QSerialPort::StopBits stopBits);
    bool setFlowControl(QSerialPort::FlowControl flowControl);

    // 数据读写
    bool sendData(const QByteArray &data);
    bool sendText(const QString &text);

    // 发送控制命令
    bool sendCommand(const Communication::Command &command);

    // 状态查询
    QString getPortName() const;
    int getBaudRate() const;
    QString getErrorString() const;

    // 静态方法
    static QStringList getStandardBaudRates();
    static QSerialPortInfo getPortInfo(const QString &portName);

signals:
    // 数据接收信号
    void dataReceived(const QByteArray &data);
    void textReceived(const QString &text);

    // 完整协议帧接收信号
    void completeFrameReceived(const QByteArray &frame);

    // 解析后的ESP32状态数据信号
    void esp32StateReceived(const Communication::ESP32State &state);

    // 状态变化信号
    void portOpened(const QString &portName);
    void portClosed();
    void errorOccurred(const QString &error);

    // 连接状态信号
    void connectionStatusChanged(bool connected);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void handleBytesWritten(qint64 bytes);

private:
    void setupConnections();
    void emitError(const QString &message);
    void processFrameBuffer(); // 处理协议帧缓冲

private:
    QSerialPort *m_serialPort;
    QString m_portName;
    int m_baudRate;
    QByteArray m_receivedData;

    // 协议解析器
    Parser::ProtocolParser m_protocolParser;

    // 协议帧缓冲
    QByteArray m_frameBuffer;
    static constexpr int PROTOCOL_FRAME_SIZE = 52; // ESP32协议帧大小
    static constexpr uint8_t FRAME_HEADER = 0xAA; // 帧头标识

    // 可选：数据缓冲和超时处理
    QTimer *m_timeoutTimer;
    static const int DEFAULT_TIMEOUT_MS = 1000;
};

}

#endif // SERIALPORTMANAGER_H
