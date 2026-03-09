#include "serialportmanager.h"
#include <QDebug>
#include <QTimer>

namespace Communication {

SerialPortManager::SerialPortManager(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_baudRate(115200)
    , m_timeoutTimer(new QTimer(this))
{
    setupConnections();

    // 设置超时定时器
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(DEFAULT_TIMEOUT_MS);
}

SerialPortManager::~SerialPortManager()
{
    if (m_serialPort->isOpen()) {
        closePort();
    }
}

void SerialPortManager::setupConnections()
{
    // 数据就绪信号
    connect(m_serialPort, &QSerialPort::readyRead,
            this, &SerialPortManager::handleReadyRead);

    // 错误处理信号
    connect(m_serialPort, &QSerialPort::errorOccurred,
            this, &SerialPortManager::handleError);

    // 数据写入完成信号
    connect(m_serialPort, &QSerialPort::bytesWritten,
            this, &SerialPortManager::handleBytesWritten);
}

QStringList SerialPortManager::getAvailablePorts() const
{
    QStringList portNames;
    const auto serialPortInfos = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &info : serialPortInfos) {
        portNames << info.portName();
    }

    return portNames;
}

bool SerialPortManager::openPort(const QString &portName, int baudRate)
{
    // 如果已经有端口打开，先关闭
    if (m_serialPort->isOpen()) {
        closePort();
    }

    // 设置端口名
    m_serialPort->setPortName(portName);
    m_portName = portName;
    m_baudRate = baudRate;

    // 配置串口参数
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // 尝试打开端口
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        emit portOpened(portName);
        emit connectionStatusChanged(true);
        return true;
    } else {
        // 详细的错误报告
        QSerialPort::SerialPortError error = m_serialPort->error();
        QString errorString = m_serialPort->errorString();

        QString detailedError = QString("❌ 串口打开失败 %1\n")
                              .arg(portName);
        detailedError += QString("错误代码: %1\n").arg(static_cast<int>(error));
        detailedError += QString("错误描述: %1\n").arg(errorString);

        // 根据错误类型提供建议
        switch (error) {
        case QSerialPort::PermissionError:
            detailedError += "建议: 检查串口权限，尝试以管理员身份运行程序";
            break;
        case QSerialPort::DeviceNotFoundError:
            detailedError += "建议: 检查设备是否正确连接，驱动是否安装";
            break;
        case QSerialPort::OpenError:
            detailedError += "建议: 检查串口是否被其他程序占用";
            break;
        case QSerialPort::ResourceError:
            detailedError += "建议: 检查USB线缆连接，尝试重新插拔设备";
            break;
        default:
            detailedError += "建议: 检查设备驱动和USB连接";
            break;
        }

        qDebug() << detailedError;
        emitError(detailedError);
        return false;
    }
}

void SerialPortManager::closePort()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        m_portName.clear();
        emit portClosed();
        emit connectionStatusChanged(false);
    }
}

bool SerialPortManager::isOpen() const
{
    return m_serialPort->isOpen();
}

bool SerialPortManager::setBaudRate(int baudRate)
{
    if (m_serialPort->setBaudRate(baudRate)) {
        m_baudRate = baudRate;
        return true;
    } else {
        emitError(QString("Failed to set baud rate %1: %2")
                  .arg(baudRate)
                  .arg(m_serialPort->errorString()));
        return false;
    }
}

bool SerialPortManager::setDataBits(QSerialPort::DataBits dataBits)
{
    if (m_serialPort->setDataBits(dataBits)) {
        return true;
    } else {
        emitError(QString("Failed to set data bits: %1")
                  .arg(m_serialPort->errorString()));
        return false;
    }
}

bool SerialPortManager::setParity(QSerialPort::Parity parity)
{
    if (m_serialPort->setParity(parity)) {
        return true;
    } else {
        emitError(QString("Failed to set parity: %1")
                  .arg(m_serialPort->errorString()));
        return false;
    }
}

bool SerialPortManager::setStopBits(QSerialPort::StopBits stopBits)
{
    if (m_serialPort->setStopBits(stopBits)) {
        return true;
    } else {
        emitError(QString("Failed to set stop bits: %1")
                  .arg(m_serialPort->errorString()));
        return false;
    }
}

bool SerialPortManager::setFlowControl(QSerialPort::FlowControl flowControl)
{
    if (m_serialPort->setFlowControl(flowControl)) {
        return true;
    } else {
        emitError(QString("Failed to set flow control: %1")
                  .arg(m_serialPort->errorString()));
        return false;
    }
}

bool SerialPortManager::sendData(const QByteArray &data)
{
    if (!m_serialPort->isOpen()) {
        emitError("Serial port is not open");
        return false;
    }

    qint64 bytesWritten = m_serialPort->write(data);
    if (bytesWritten == -1) {
        emitError(QString("Failed to write data: %1")
                  .arg(m_serialPort->errorString()));
        return false;
    } else if (bytesWritten != data.size()) {
        emitError(QString("Not all data written: %1/%2 bytes")
                  .arg(bytesWritten)
                  .arg(data.size()));
        return false;
    }

    return m_serialPort->flush();
}

bool SerialPortManager::sendText(const QString &text)
{
    return sendData(text.toUtf8());
}

bool SerialPortManager::sendCommand(const Communication::Command &command)
{
    // 使用ProtocolParser编码命令
    QByteArray encodedFrame = m_protocolParser.encodeToQByteArray(command);
    if (encodedFrame.isEmpty()) {
        emitError("Failed to encode command to protocol frame");
        return false;
    }

    // 发送编码后的帧
    return sendData(encodedFrame);
}

QString SerialPortManager::getPortName() const
{
    return m_portName;
}

int SerialPortManager::getBaudRate() const
{
    return m_baudRate;
}

QString SerialPortManager::getErrorString() const
{
    return m_serialPort->errorString();
}

QStringList SerialPortManager::getStandardBaudRates()
{
    return {
        "1200", "2400", "4800", "9600",
        "19200", "38400", "57600", "115200",
        "230400", "460800", "921600"
    };
}

QSerialPortInfo SerialPortManager::getPortInfo(const QString &portName)
{
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : serialPortInfos) {
        if (info.portName() == portName) {
            return info;
        }
    }
    return QSerialPortInfo(); // 返回空的信息对象
}

void SerialPortManager::handleReadyRead()
{
    // 读取所有可用数据
    QByteArray data = m_serialPort->readAll();

    m_receivedData.append(data);
    m_frameBuffer.append(data);

    // 发送原始数据信号
    emit dataReceived(data);

    // 如果数据包含完整行，发送文本信号
    while (m_receivedData.contains('\n')) {
        int index = m_receivedData.indexOf('\n');
        QString line = QString::fromUtf8(m_receivedData.left(index));
        m_receivedData.remove(0, index + 1);
        emit textReceived(line.trimmed());
    }

    // 处理协议帧缓冲
    processFrameBuffer();

    // 启动超时定时器（如果需要）
    m_timeoutTimer->start();
}

void SerialPortManager::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return; // 无错误，直接返回
    }

    QString errorMessage;

    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        errorMessage = "Device not found";
        break;
    case QSerialPort::PermissionError:
        errorMessage = "Permission denied";
        break;
    case QSerialPort::OpenError:
        errorMessage = "Error opening device";
        break;
    case QSerialPort::NotOpenError:
        errorMessage = "Device is not open";
        break;
    case QSerialPort::WriteError:
        errorMessage = "Write error";
        break;
    case QSerialPort::ReadError:
        errorMessage = "Read error";
        break;
    case QSerialPort::ResourceError:
        errorMessage = "Resource error (device disconnected?)";
        break;
    case QSerialPort::UnsupportedOperationError:
        errorMessage = "Unsupported operation";
        break;
    case QSerialPort::TimeoutError:
        errorMessage = "Timeout error";
        break;
    default:
        errorMessage = QString("Unknown error: %1").arg(error);
        break;
    }

    emitError(errorMessage);

    // 如果是严重错误，关闭端口
    if (error == QSerialPort::ResourceError ||
        error == QSerialPort::DeviceNotFoundError) {
        closePort();
    }
}

void SerialPortManager::handleBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes)
}

void SerialPortManager::processFrameBuffer()
{
    // 当缓冲区数据不足一个完整帧时，直接返回
    while (m_frameBuffer.size() >= PROTOCOL_FRAME_SIZE) {
        // 查找帧头 (0xAA)
        int headerIndex = m_frameBuffer.indexOf(FRAME_HEADER);

        if (headerIndex == -1) {
            qDebug() << "[SerialPortManager] 未找到帧头，缓冲区内容:" << m_frameBuffer.toHex(' ');
            // 没有找到帧头，清空缓冲区（保留最后PROTOCOL_FRAME_SIZE-1字节）
            if (m_frameBuffer.size() > PROTOCOL_FRAME_SIZE - 1) {
                m_frameBuffer = m_frameBuffer.right(PROTOCOL_FRAME_SIZE - 1);
            }
            return;
        }

        // 如果帧头不在缓冲区开头，丢弃帧头之前的数据
        if (headerIndex > 0) {
            m_frameBuffer.remove(0, headerIndex);
        }

        // 检查是否有足够的字节构成完整帧
        if (m_frameBuffer.size() < PROTOCOL_FRAME_SIZE) {
            return; // 等待更多数据
        }

        // 提取一个完整的帧
        QByteArray frame = m_frameBuffer.left(PROTOCOL_FRAME_SIZE);

        // 发送完整帧信号
        emit completeFrameReceived(frame);

        // 使用ProtocolParser解析帧数据
        Communication::ESP32State state;
        if (m_protocolParser.parseFromQByteArray(frame, &state)) {
            // 发送解析后的ESP32状态信号
            emit esp32StateReceived(state);
        }

        // 从缓冲区移除已处理的帧
        m_frameBuffer.remove(0, PROTOCOL_FRAME_SIZE);
    }
}

void SerialPortManager::emitError(const QString &message)
{
    emit errorOccurred(message);
}

} // namespace Communication