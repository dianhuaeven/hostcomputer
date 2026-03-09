#include "MultiCameraReceiver.h"
#include "SingleCameraReceiver.h"
#include <QDebug>
#include <QThread>

namespace Communication {

MultiCameraReceiver::MultiCameraReceiver(QObject *parent)
    : QObject(parent)
{
    // 创建6个摄像头接收器，每个在独立线程中运行
    for (int i = 0; i < 6; ++i) {
        // 创建线程
        QThread* thread = new QThread(this);
        thread->setObjectName(QString("Camera%1Thread").arg(i));

        // 创建接收器
        SingleCameraReceiver* receiver = new SingleCameraReceiver(i);
        receiver->moveToThread(thread);

        // 保存到智能指针（注意：不拥有所有权，由thread管理）
        m_receivers[i].reset(receiver);

        // 连接信号转发（使用QueuedConnection避免死锁）
        connect(receiver, &SingleCameraReceiver::frameReceived,
                this, &MultiCameraReceiver::frameReceived, Qt::QueuedConnection);
        connect(receiver, &SingleCameraReceiver::jpegDataReceived,
                this, &MultiCameraReceiver::jpegDataReceived, Qt::QueuedConnection);
        connect(receiver, &SingleCameraReceiver::listeningChanged,
                this, &MultiCameraReceiver::listeningChanged, Qt::QueuedConnection);
        connect(receiver, &SingleCameraReceiver::errorOccurred,
                this, &MultiCameraReceiver::errorOccurred, Qt::QueuedConnection);

        // stats信号需要类型转换，通过中间槽函数处理
        connect(receiver, &SingleCameraReceiver::statsUpdated,
                this, [this, i](const SingleCameraReceiver::Stats &, int) {
                    onInternalStatsUpdated(i);
                }, Qt::QueuedConnection);

        // 启动线程
        thread->start();

        qDebug() << "MultiCameraReceiver: 摄像头" << i << "线程已启动";
    }

    qDebug() << "MultiCameraReceiver: 创建了6个摄像头接收器";
}

MultiCameraReceiver::~MultiCameraReceiver()
{
    stopAll();

    // 停止所有线程
    for (int i = 0; i < 6; ++i) {
        QThread* thread = m_receivers[i]->thread();
        if (thread && thread->isRunning()) {
            thread->quit();
            thread->wait(3000);
        }
    }
}

int MultiCameraReceiver::startAll(quint16 basePort, const QHostAddress &bindAddress)
{
    int startedCount = 0;

    for (int i = 0; i < 6; ++i) {
        if (startCamera(i, basePort + i, bindAddress)) {
            startedCount++;
        }
    }

    qDebug() << QString("MultiCameraReceiver: 启动了 %1/%2 个摄像头")
                .arg(startedCount).arg(6);

    return startedCount;
}

bool MultiCameraReceiver::startCamera(int cameraId, quint16 port, const QHostAddress &bindAddress)
{
    if (cameraId < 0 || cameraId >= 6) {
        qWarning() << "MultiCameraReceiver: 无效的摄像头ID:" << cameraId;
        return false;
    }

    SingleCameraReceiver* receiver = m_receivers[cameraId].get();
    if (!receiver) {
        qWarning() << "MultiCameraReceiver: 摄像头接收器未创建:" << cameraId;
        return false;
    }

    // 使用 QMetaObject::invokeMethod 跨线程调用
    bool result = false;
    QMetaObject::invokeMethod(receiver, [receiver, port, bindAddress, &result]() {
        result = receiver->start(port, bindAddress);
    }, Qt::BlockingQueuedConnection);

    return result;
}

void MultiCameraReceiver::stopCamera(int cameraId)
{
    if (cameraId < 0 || cameraId >= 6) {
        return;
    }

    SingleCameraReceiver* receiver = m_receivers[cameraId].get();
    if (!receiver) {
        return;
    }

    // 跨线程调用
    QMetaObject::invokeMethod(receiver, [receiver]() {
        receiver->stop();
    }, Qt::BlockingQueuedConnection);
}

void MultiCameraReceiver::stopAll()
{
    for (int i = 0; i < 6; ++i) {
        stopCamera(i);
    }

    qDebug() << "MultiCameraReceiver: 停止所有摄像头监听";
}

bool MultiCameraReceiver::isCameraListening(int cameraId) const
{
    if (cameraId < 0 || cameraId >= 6) {
        return false;
    }

    SingleCameraReceiver* receiver = m_receivers[cameraId].get();
    if (!receiver) {
        return false;
    }

    // 跨线程调用
    bool listening = false;
    QMetaObject::invokeMethod(
        receiver,
        [receiver, &listening]() {
            listening = receiver->isListening();
        },
        Qt::BlockingQueuedConnection
    );

    return listening;
}

MultiCameraReceiver::CameraStats MultiCameraReceiver::getCameraStats(int cameraId) const
{
    CameraStats result;

    if (cameraId < 0 || cameraId >= 6) {
        return result;
    }

    SingleCameraReceiver* receiver = m_receivers[cameraId].get();
    if (!receiver) {
        return result;
    }

    SingleCameraReceiver::Stats internalStats;
    QMetaObject::invokeMethod(
        receiver,
        [receiver, &internalStats]() {
            internalStats = receiver->getStats();
        },
        Qt::BlockingQueuedConnection
    );

    // 转换为公开类型
    result.framesReceived = internalStats.framesReceived;
    result.framesDropped = internalStats.framesDropped;
    result.packetsReceived = internalStats.packetsReceived;
    result.packetsLost = internalStats.packetsLost;
    result.bytesReceived = internalStats.bytesReceived;

    return result;
}

void MultiCameraReceiver::resetCameraStats(int cameraId)
{
    if (cameraId < 0 || cameraId >= 6) {
        return;
    }

    SingleCameraReceiver* receiver = m_receivers[cameraId].get();
    if (!receiver) {
        return;
    }

    QMetaObject::invokeMethod(receiver, [receiver]() {
        receiver->resetStats();
    }, Qt::BlockingQueuedConnection);
}

void MultiCameraReceiver::setTimeout(int cameraId, int ms)
{
    if (cameraId < 0 || cameraId >= 6) {
        return;
    }

    SingleCameraReceiver* receiver = m_receivers[cameraId].get();
    if (!receiver) {
        return;
    }

    QMetaObject::invokeMethod(receiver, [receiver, ms]() {
        receiver->setTimeout(ms);
    }, Qt::BlockingQueuedConnection);
}

void MultiCameraReceiver::onInternalStatsUpdated(int cameraId)
{
    if (cameraId < 0 || cameraId >= 6) {
        return;
    }

    // 获取内部stats并转换为公开类型
    CameraStats publicStats = getCameraStats(cameraId);
    emit statsUpdated(publicStats, cameraId);
}

} // namespace Communication
