#ifndef SINGLE_CAMERA_RECEIVER_H
#define SINGLE_CAMERA_RECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QImage>
#include <QTimer>
#include <QMutex>
#include <QBuffer>
#include <array>
#include "SharedStructs.h"

namespace Communication {

/**
 * @brief 单摄像头视频接收器（运行在独立线程）
 *
 * 每个摄像头实例运行在独立线程中，由 NetworkThreadManager 管理
 */
class SingleCameraReceiver : public QObject
{
    Q_OBJECT

public:
    explicit SingleCameraReceiver(int cameraId, QObject *parent = nullptr);
    ~SingleCameraReceiver();

    bool start(quint16 port, const QHostAddress &bindAddress = QHostAddress::Any);
    void stop();
    bool isListening() const;
    int getCameraId() const { return m_cameraId; }
    void setTimeout(int ms);

    struct Stats {
        quint64 framesReceived;
        quint64 framesDropped;
        quint64 packetsReceived;
        quint64 packetsLost;
        quint64 bytesReceived;
    };
    Stats getStats() const;
    void resetStats();

signals:
    void frameReceived(const QImage &image, int cameraId);
    void jpegDataReceived(const QByteArray &jpegData, int cameraId);
    void listeningChanged(bool listening, int cameraId);
    void errorOccurred(const QString &error, int cameraId);
    void statsUpdated(const Stats &stats, int cameraId);

public slots:
    void processPendingDatagrams();

private:
    void processDatagram(const QByteArray &datagram, const QHostAddress &sender);
    bool parseFrameHeader(const QByteArray &data, MjpegFrameHeader &header);
    void handleHeaderPacket(const MjpegFrameHeader &header);
    void handleDataPacket(const MjpegFrameHeader &header, const QByteArray &payload);
    void completeFrame(uint16_t frameId);
    void cleanupStaleBuffers();
    void clearFrameBuffer();
    QImage imageFromJpeg(const QByteArray &jpegData);
    void emitStatsUpdate();

private:
    int m_cameraId;
    QUdpSocket *m_udpSocket;
    quint16 m_port;
    bool m_listening;
    int m_timeoutMs;
    size_t m_maxFrameBufferSize;

    Stats m_stats;

    struct FrameBuffer {
        uint16_t frameId;
        uint16_t totalPackets;
        uint32_t payloadSize;
        QByteArray data;
        std::vector<bool> receivedPackets;
        qint64 lastUpdateTime;
    };
    FrameBuffer m_frameBuffer;

    mutable QMutex m_bufferMutex;
    QTimer *m_heartbeatTimer;

    static constexpr uint8_t FRAME_MAGIC = 0xAA;
    static constexpr int DEFAULT_TIMEOUT_MS = 500;
    static constexpr size_t DEFAULT_MAX_FRAME_SIZE = 10 * 1024 * 1024;
};

} // namespace Communication

#endif // SINGLE_CAMERA_RECEIVER_H
