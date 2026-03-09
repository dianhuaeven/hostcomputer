#ifndef MULTI_CAMERA_RECEIVER_H
#define MULTI_CAMERA_RECEIVER_H

#include <QObject>
#include <QHostAddress>
#include <QImage>
#include <QByteArray>
#include <memory>
#include <array>
#include <cstdint>

namespace Communication {

// 前向声明，内部实现使用
class SingleCameraReceiver;

/**
 * @brief 多摄像头接收器管理类
 *
 * 管理6个独立的摄像头接收器，每个运行在独立线程中：
 * - Camera0: 线程0
 * - Camera1: 线程1
 * - Camera2: 线程2
 * - Camera3: 线程3
 * - Camera4: 线程4
 * - Camera5: 线程5
 */
class MultiCameraReceiver : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 摄像头统计信息（对外暴露，隐藏内部实现）
     */
    struct CameraStats {
        quint64 framesReceived = 0;
        quint64 framesDropped = 0;
        quint64 packetsReceived = 0;
        quint64 packetsLost = 0;
        quint64 bytesReceived = 0;
    };

    explicit MultiCameraReceiver(QObject *parent = nullptr);
    ~MultiCameraReceiver();

    int startAll(quint16 basePort, const QHostAddress &bindAddress = QHostAddress::Any);
    bool startCamera(int cameraId, quint16 port, const QHostAddress &bindAddress = QHostAddress::Any);
    void stopCamera(int cameraId);
    void stopAll();
    bool isCameraListening(int cameraId) const;
    CameraStats getCameraStats(int cameraId) const;
    void resetCameraStats(int cameraId);
    void setTimeout(int cameraId, int ms);

signals:
    void frameReceived(const QImage &image, int cameraId);
    void jpegDataReceived(const QByteArray &jpegData, int cameraId);
    void listeningChanged(bool listening, int cameraId);
    void errorOccurred(const QString &error, int cameraId);
    void statsUpdated(const CameraStats &stats, int cameraId);

private slots:
    void onInternalStatsUpdated(int cameraId);

private:
    std::array<std::unique_ptr<SingleCameraReceiver>, 6> m_receivers;
};

} // namespace Communication

#endif // MULTI_CAMERA_RECEIVER_H
