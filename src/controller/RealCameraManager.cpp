#include "RealCameraManager.h"
#include <QMediaDevices>
#include <QCameraDevice>
#include <QVideoFrameFormat>
#include <QImage>
#include <QDebug>
#include <QDateTime>

RealCameraManager::RealCameraManager(QObject *parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
    , m_isInitialized(false)
{
    // 设置摄像头列表刷新定时器
    m_refreshTimer->setInterval(5000); // 5秒检查一次摄像头变化
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        QStringList cameras = getAvailableCameras();
        emit cameraListChanged(cameras);
    });
    m_refreshTimer->start();

    qDebug() << "RealCameraManager initialized";
}

RealCameraManager::~RealCameraManager()
{
    stopAllCameras();
    qDebug() << "RealCameraManager destroyed";
}

QStringList RealCameraManager::getAvailableCameras() const
{
    QStringList cameraNames;
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();

    for (const QCameraDevice &camera : cameras) {
        cameraNames << camera.description();
        qDebug() << "Found camera:" << camera.description() << camera.id();
    }

    return cameraNames;
}

bool RealCameraManager::startCamera(int cameraId, QWidget* displayWidget)
{
    if (!displayWidget) {
        qWarning() << "Display widget is null for camera" << cameraId;
        return false;
    }

    // 停止已有的摄像头
    if (m_cameras.contains(cameraId)) {
        stopCamera(cameraId);
    }

    // 获取可用的摄像头列表
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        emit cameraError(cameraId, "没有找到可用的摄像头");
        return false;
    }

    // 选择摄像头（如果摄像头ID超出范围，使用第一个摄像头）
    int cameraIndex = (cameraId - 1) % cameras.size();
    const QCameraDevice &cameraDevice = cameras[cameraIndex];

    // 创建摄像头组件
    QCamera* camera = new QCamera(cameraDevice, this);
    QMediaCaptureSession* captureSession = new QMediaCaptureSession(this);
    QVideoSink* videoSink = new QVideoSink(this);

    // 设置摄像头组件
    captureSession->setCamera(camera);
    captureSession->setVideoSink(videoSink);

    // 存储摄像头信息
    CameraInfo info;
    info.name = cameraDevice.description();
    info.description = QString("Device: %1").arg(cameraDevice.id());
    info.isRunning = true;
    info.displayWidget = displayWidget;
    info.camera = camera;
    info.captureSession = captureSession;
    info.videoSink = videoSink;

    m_cameras[cameraId] = info;

    // 连接视频帧接收信号
    connect(videoSink, &QVideoSink::videoFrameChanged, this,
            [this, cameraId](const QVideoFrame &frame) {
                onVideoFrameReceived(frame);
            });

    // 连接摄像头错误信号
    connect(camera, &QCamera::errorChanged, this, &RealCameraManager::onCameraError);
    connect(camera, &QCamera::activeChanged, this, &RealCameraManager::onCameraStatusChanged);

    // 启动摄像头
    camera->start();

    qDebug() << "Camera" << cameraId << "started:" << info.name;
    emit cameraStarted(cameraId, info.name);

    return true;
}

void RealCameraManager::stopCamera(int cameraId)
{
    if (!m_cameras.contains(cameraId)) {
        return;
    }

    CameraInfo &info = m_cameras[cameraId];

    if (info.camera && info.camera->isActive()) {
        info.camera->stop();
    }

    // 删除组件
    if (info.camera) {
        info.camera->deleteLater();
    }
    if (info.captureSession) {
        info.captureSession->deleteLater();
    }
    if (info.videoSink) {
        info.videoSink->deleteLater();
    }

    info.isRunning = false;
    info.displayWidget = nullptr;
    info.camera = nullptr;
    info.captureSession = nullptr;
    info.videoSink = nullptr;

    qDebug() << "Camera" << cameraId << "stopped";
    emit cameraStopped(cameraId);

    // 从列表中移除
    m_cameras.remove(cameraId);
}

void RealCameraManager::stopAllCameras()
{
    // 复制key列表避免迭代时修改容器
    QList<int> cameraIds = m_cameras.keys();
    for (int cameraId : cameraIds) {
        stopCamera(cameraId);
    }
}

bool RealCameraManager::isCameraRunning(int cameraId) const
{
    return m_cameras.contains(cameraId) && m_cameras[cameraId].isRunning;
}

int RealCameraManager::getCameraCount() const
{
    return m_cameras.size();
}

void RealCameraManager::onVideoFrameReceived(const QVideoFrame &frame)
{
    // 找到对应的摄像头ID
    QObject* sender = QObject::sender();
    int cameraId = -1;

    for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it) {
        if (it.value().videoSink == sender) {
            cameraId = it.key();
            break;
        }
    }

    if (cameraId == -1 || !m_cameras[cameraId].displayWidget) {
        return;
    }

    // 处理视频帧
    processVideoFrame(frame, cameraId);
}

void RealCameraManager::processVideoFrame(const QVideoFrame &frame, int cameraId)
{
    if (!m_cameras.contains(cameraId) || !m_cameras[cameraId].displayWidget) {
        return;
    }

    static int frameCount = 0;
    frameCount++;

    // 每100帧打印一次调试信息
    if (frameCount % 100 == 0) {
        qDebug() << "Processing frame" << frameCount << "for camera" << cameraId
                 << "Frame size:" << frame.width() << "x" << frame.height()
                 << "PixelFormat:" << frame.pixelFormat();
    }

    // TODO: 将视频帧传递给新的显示 widget
    Q_UNUSED(frame);
}

QPixmap RealCameraManager::videoFrameToPixmap(const QVideoFrame &frame)
{
    if (!frame.isValid()) {
        return QPixmap();
    }

    // 使用QVideoFrame的toImage()方法（Qt6推荐的方法）
    QImage image = frame.toImage();

    if (image.isNull()) {
        qWarning() << "Failed to convert frame to image";
        return QPixmap();
    }

    // 转换为RGB32格式
    if (image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_RGB32);
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    if (pixmap.isNull()) {
        qWarning() << "Failed to create pixmap from image";
        return QPixmap();
    }

    return pixmap;
}

void RealCameraManager::onCameraError()
{
    QCamera* camera = qobject_cast<QCamera*>(sender());
    if (!camera) return;

    // 找到对应的摄像头ID
    int cameraId = -1;
    for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it) {
        if (it.value().camera == camera) {
            cameraId = it.key();
            break;
        }
    }

    if (cameraId != -1) {
        QString errorMessage = camera->errorString();
        qWarning() << "Camera" << cameraId << "error:" << errorMessage;
        emit cameraError(cameraId, errorMessage);

        // 停止有问题的摄像头
        stopCamera(cameraId);
    }
}

void RealCameraManager::onCameraStatusChanged()
{
    QCamera* camera = qobject_cast<QCamera*>(sender());
    if (!camera) return;

    // 可以在这里处理摄像头状态变化
    qDebug() << "Camera status changed:" << camera->isActive();
}