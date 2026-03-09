#ifndef REAL_CAMERA_MANAGER_H
#define REAL_CAMERA_MANAGER_H

#include <QObject>
#include <QCamera>
#include <QVideoWidget>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QPixmap>
#include <QTimer>
#include <QMap>

class QVideoWidget;

/**
 * @brief 真实摄像头管理器 - 使用本机摄像头显示画面
 */
class RealCameraManager : public QObject
{
    Q_OBJECT

public:
    explicit RealCameraManager(QObject *parent = nullptr);
    ~RealCameraManager();

    // 摄像头管理
    bool startCamera(int cameraId, QWidget* displayWidget);
    void stopCamera(int cameraId);
    void stopAllCameras();

    // 状态查询
    bool isCameraRunning(int cameraId) const;
    int getCameraCount() const;
    QStringList getAvailableCameras() const;

signals:
    // 状态信号
    void cameraStarted(int cameraId, const QString& cameraName);
    void cameraStopped(int cameraId);
    void cameraError(int cameraId, const QString& error);

    // 列表更新信号
    void cameraListChanged(const QStringList& cameraNames);

private slots:
    void onVideoFrameReceived(const QVideoFrame &frame);
    void onCameraError();
    void onCameraStatusChanged();

private:
    void setupCamera(int cameraId);
    void processVideoFrame(const QVideoFrame &frame, int cameraId);
    QPixmap videoFrameToPixmap(const QVideoFrame &frame);

private:
    struct CameraInfo {
        QString name;
        QString description;
        bool isRunning;
        QWidget* displayWidget;
        QCamera* camera;
        QMediaCaptureSession* captureSession;
        QVideoSink* videoSink;
    };

    QMap<int, CameraInfo> m_cameras;
    QTimer* m_refreshTimer;
    bool m_isInitialized;
};

#endif // REAL_CAMERA_MANAGER_H