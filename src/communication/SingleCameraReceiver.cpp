#include "SingleCameraReceiver.h"
#include <QCoreApplication>
#include <QDebug>
#include <QImageReader>
#include <QDateTime>
#include <algorithm>

namespace Communication {

SingleCameraReceiver::SingleCameraReceiver(int cameraId, QObject *parent)
    : QObject(parent)
    , m_cameraId(cameraId)
    , m_udpSocket(nullptr)
    , m_port(0)
    , m_listening(false)
    , m_timeoutMs(DEFAULT_TIMEOUT_MS)
    , m_maxFrameBufferSize(DEFAULT_MAX_FRAME_SIZE)
    , m_heartbeatTimer(nullptr)
{
    // 验证摄像头ID
    if (m_cameraId < 0 || m_cameraId >= 6) {
        qWarning() << "SingleCameraReceiver: 无效的摄像头ID:" << m_cameraId;
        m_cameraId = 0;
    }

    // 初始化统计信息
    m_stats.framesReceived = 0;
    m_stats.framesDropped = 0;
    m_stats.packetsReceived = 0;
    m_stats.packetsLost = 0;
    m_stats.bytesReceived = 0;

    // 初始化帧缓冲
    m_frameBuffer.frameId = 0;
    m_frameBuffer.totalPackets = 0;
    m_frameBuffer.payloadSize = 0;
    m_frameBuffer.data.clear();
    m_frameBuffer.receivedPackets.clear();
    m_frameBuffer.lastUpdateTime = 0;

    // 创建UDP套接字（必须在线程中创建）
    m_udpSocket = new QUdpSocket(this);

    // 创建心跳定时器
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &SingleCameraReceiver::cleanupStaleBuffers);
}

SingleCameraReceiver::~SingleCameraReceiver()
{
    stop();
}

bool SingleCameraReceiver::start(quint16 port, const QHostAddress &bindAddress)
{
    if (m_listening) {
        stop();
    }

    if (m_udpSocket->bind(bindAddress, port)) {
        m_port = port;
        m_listening = true;

        connect(m_udpSocket, &QUdpSocket::readyRead,
                this, &SingleCameraReceiver::processPendingDatagrams);

        // 启动心跳定时器
        m_heartbeatTimer->start(100);

        emit listeningChanged(true, m_cameraId);
        qDebug() << QString("SingleCameraReceiver[%1]: Started listening on port %2")
                    .arg(m_cameraId).arg(port);
        return true;
    } else {
        QString error = QString("Failed to bind to port %1: %2")
                            .arg(port)
                            .arg(m_udpSocket->errorString());
        emit errorOccurred(error, m_cameraId);
        qDebug() << "SingleCameraReceiver[" << m_cameraId << "]:" << error;
        return false;
    }
}

void SingleCameraReceiver::stop()
{
    if (m_listening) {
        disconnect(m_udpSocket, &QUdpSocket::readyRead,
                   this, &SingleCameraReceiver::processPendingDatagrams);

        m_udpSocket->close();
        m_heartbeatTimer->stop();
        m_listening = false;
        m_port = 0;

        // 清空帧缓冲
        QMutexLocker locker(&m_bufferMutex);
        m_frameBuffer.data.clear();
        m_frameBuffer.receivedPackets.clear();

        emit listeningChanged(false, m_cameraId);
        qDebug() << "SingleCameraReceiver[" << m_cameraId << "]: Stopped";
    }
}

bool SingleCameraReceiver::isListening() const
{
    return m_listening;
}

void SingleCameraReceiver::setTimeout(int ms)
{
    m_timeoutMs = ms;
}

SingleCameraReceiver::Stats SingleCameraReceiver::getStats() const
{
    return m_stats;
}

void SingleCameraReceiver::resetStats()
{
    m_stats.framesReceived = 0;
    m_stats.framesDropped = 0;
    m_stats.packetsReceived = 0;
    m_stats.packetsLost = 0;
    m_stats.bytesReceived = 0;
    emitStatsUpdate();
}

void SingleCameraReceiver::processPendingDatagrams()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_udpSocket->pendingDatagramSize()));

        QHostAddress sender;
        quint16 senderPort;

        qint64 size = m_udpSocket->readDatagram(datagram.data(), datagram.size(),
                                                &sender, &senderPort);

        if (size > 0) {
            m_stats.bytesReceived += size;
            processDatagram(datagram, sender);
        }
    }

    emitStatsUpdate();
}

void SingleCameraReceiver::processDatagram(const QByteArray &datagram, const QHostAddress &sender)
{
    Q_UNUSED(sender);

    // 数据包最小长度检查
    if (datagram.size() < static_cast<int>(sizeof(MjpegFrameHeader))) {
        qDebug() << "SingleCameraReceiver[" << m_cameraId << "]: Packet too small, size =" << datagram.size();
        return;
    }

    // 解析帧头
    MjpegFrameHeader header;
    if (!parseFrameHeader(datagram, header)) {
        return;
    }

    // 检查摄像头ID是否匹配
    if (header.cameraId != static_cast<uint8_t>(m_cameraId)) {
        // 不是本摄像头的数据，忽略
        return;
    }

    m_stats.packetsReceived++;

    // 根据帧类型处理
    switch (static_cast<MjpegFrameType>(header.frameType)) {
        case MjpegFrameType::Header:
            handleHeaderPacket(header);
            break;

        case MjpegFrameType::Data:
            {
                int payloadOffset = sizeof(MjpegFrameHeader);
                QByteArray payload = datagram.mid(payloadOffset);
                handleDataPacket(header, payload);
            }
            break;

        case MjpegFrameType::End:
            completeFrame(header.frameId);
            break;

        case MjpegFrameType::Heartbeat:
            // 心跳包，不需要特殊处理
            break;

        default:
            qDebug() << "SingleCameraReceiver[" << m_cameraId << "]: Unknown frame type:" << header.frameType;
            break;
    }
}

bool SingleCameraReceiver::parseFrameHeader(const QByteArray &data, MjpegFrameHeader &header)
{
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data.constData());

    header.magic = ptr[0];

    // 检查魔数
    if (header.magic != FRAME_MAGIC) {
        return false;
    }

    header.frameType = ptr[1];
    header.frameId = qFromLittleEndian<quint16>(ptr + 2);
    header.packetIndex = qFromLittleEndian<quint16>(ptr + 4);
    header.totalPackets = qFromLittleEndian<quint16>(ptr + 6);
    header.payloadSize = qFromLittleEndian<quint32>(ptr + 8);
    header.timestamp = qFromLittleEndian<quint32>(ptr + 12);
    header.cameraId = ptr[16];

    return true;
}

void SingleCameraReceiver::handleHeaderPacket(const MjpegFrameHeader &header)
{
    QMutexLocker locker(&m_bufferMutex);

    // 如果有未完成的帧，先丢弃
    if (!m_frameBuffer.data.isEmpty() && m_frameBuffer.frameId != header.frameId) {
        m_stats.framesDropped++;
        qDebug() << "SingleCameraReceiver[" << m_cameraId << "]: Dropped incomplete frame" << m_frameBuffer.frameId;
    }

    // 初始化新的帧缓冲
    m_frameBuffer.frameId = header.frameId;
    m_frameBuffer.totalPackets = header.totalPackets;
    m_frameBuffer.payloadSize = qMin(static_cast<uint32_t>(m_maxFrameBufferSize), header.payloadSize);
    m_frameBuffer.data.clear();
    m_frameBuffer.data.resize(m_frameBuffer.payloadSize);
    m_frameBuffer.receivedPackets.assign(header.totalPackets, false);
    m_frameBuffer.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
}

void SingleCameraReceiver::handleDataPacket(const MjpegFrameHeader &header, const QByteArray &payload)
{
    QMutexLocker locker(&m_bufferMutex);

    // 检查是否是当前帧的数据包
    if (m_frameBuffer.frameId != header.frameId) {
        // 如果是旧帧的包，忽略
        if (static_cast<int16_t>(header.frameId - m_frameBuffer.frameId) < 0) {
            return;
        }

        // 如果是新帧的包，但还没收到帧头，先初始化
        if (m_frameBuffer.data.isEmpty()) {
            m_frameBuffer.frameId = header.frameId;
            m_frameBuffer.totalPackets = header.totalPackets;
            m_frameBuffer.payloadSize = header.payloadSize;
            m_frameBuffer.data.resize(m_frameBuffer.payloadSize);
            m_frameBuffer.receivedPackets.assign(header.totalPackets, false);
        }
    }

    // 检查分片索引有效性
    if (header.packetIndex >= m_frameBuffer.receivedPackets.size()) {
        return;
    }

    // 检查是否已接收过此分片
    if (m_frameBuffer.receivedPackets[header.packetIndex]) {
        return; // 重复包，忽略
    }

    // 计算写入位置
    size_t offset = header.packetIndex * 1400;
    size_t remainingSize = m_frameBuffer.payloadSize - offset;

    // 写入数据
    size_t copySize = qMin(static_cast<size_t>(payload.size()), remainingSize);
    if (copySize > 0) {
        std::memcpy(m_frameBuffer.data.data() + offset, payload.constData(), copySize);
        m_frameBuffer.receivedPackets[header.packetIndex] = true;
        m_frameBuffer.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    }
}

void SingleCameraReceiver::completeFrame(uint16_t frameId)
{
    QMutexLocker locker(&m_bufferMutex);

    // 检查帧ID匹配
    if (m_frameBuffer.frameId != frameId) {
        return;
    }

    // 检查是否接收了所有分片
    bool allReceived = std::all_of(m_frameBuffer.receivedPackets.begin(),
                                   m_frameBuffer.receivedPackets.end(),
                                   [](bool received) { return received; });

    if (!allReceived) {
        // 未完全接收，统计丢失的包
        for (size_t i = 0; i < m_frameBuffer.receivedPackets.size(); ++i) {
            if (!m_frameBuffer.receivedPackets[i]) {
                m_stats.packetsLost++;
            }
        }
    }

    // 提取JPEG数据
    QByteArray jpegData = m_frameBuffer.data.left(static_cast<int>(m_frameBuffer.payloadSize));

    // 清空缓冲
    m_frameBuffer.data.clear();
    m_frameBuffer.receivedPackets.clear();

    locker.unlock();

    // 发射信号
    if (!jpegData.isEmpty()) {
        emit jpegDataReceived(jpegData, m_cameraId);

        // 尝试解码为图像
        QImage image = imageFromJpeg(jpegData);
        if (!image.isNull()) {
            m_stats.framesReceived++;
            emit frameReceived(image, m_cameraId);
        } else {
            m_stats.framesDropped++;
        }
    }

    emitStatsUpdate();
}

void SingleCameraReceiver::cleanupStaleBuffers()
{
    if (!m_listening) {
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    QMutexLocker locker(&m_bufferMutex);

    if (!m_frameBuffer.data.isEmpty()) {
        qint64 elapsed = currentTime - m_frameBuffer.lastUpdateTime;

        // 如果超时，清空缓冲
        if (elapsed > m_timeoutMs) {
            qDebug() << "SingleCameraReceiver[" << m_cameraId << "]: Cleaning up stale buffer - frame" << m_frameBuffer.frameId;
            clearFrameBuffer();
            m_stats.framesDropped++;
        }
    }

    emitStatsUpdate();
}

void SingleCameraReceiver::clearFrameBuffer()
{
    m_frameBuffer.data.clear();
    m_frameBuffer.receivedPackets.clear();
    m_frameBuffer.frameId = 0;
}

QImage SingleCameraReceiver::imageFromJpeg(const QByteArray &jpegData)
{
    QBuffer buffer;
    buffer.setData(jpegData);
    buffer.open(QIODevice::ReadOnly);

    QImageReader reader(&buffer, "JPEG");
    reader.setAutoTransform(true);

    QImage image = reader.read();
    return image;
}

void SingleCameraReceiver::emitStatsUpdate()
{
    emit statsUpdated(m_stats, m_cameraId);
}

} // namespace Communication
