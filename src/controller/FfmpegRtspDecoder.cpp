#include "FfmpegRtspDecoder.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

constexpr int kBytesPerPixel = 3;
constexpr int kMaxFrameBytes = 3840 * 2160 * kBytesPerPixel;
constexpr int kStderrLimit = 4000;

} // namespace

FfmpegRtspDecoder::FfmpegRtspDecoder(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::started, this, &FfmpegRtspDecoder::started);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &FfmpegRtspDecoder::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &FfmpegRtspDecoder::onReadyReadStandardError);
    connect(m_process, &QProcess::errorOccurred,
            this, &FfmpegRtspDecoder::onProcessError);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &FfmpegRtspDecoder::onProcessFinished);
}

FfmpegRtspDecoder::~FfmpegRtspDecoder()
{
    stop();
}

bool FfmpegRtspDecoder::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

void FfmpegRtspDecoder::start(const QString &rtspUrl, int width, int height, int fps)
{
    stop();

    if (rtspUrl.trimmed().isEmpty()) {
        emit failed(QStringLiteral("RTSP URL 为空"));
        return;
    }
    if (width <= 0 || height <= 0) {
        emit failed(QStringLiteral("视频尺寸无效: %1x%2").arg(width).arg(height));
        return;
    }

    const qint64 frameBytes = static_cast<qint64>(width) * height * kBytesPerPixel;
    if (frameBytes <= 0 || frameBytes > kMaxFrameBytes) {
        emit failed(QStringLiteral("视频帧过大: %1x%2").arg(width).arg(height));
        return;
    }

    m_width = width;
    m_height = height;
    m_fps = fps > 0 ? fps : 30;
    m_frameBytes = static_cast<int>(frameBytes);
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();
    m_stopRequested = false;

    m_process->start(ffmpegProgram(), ffmpegArguments(rtspUrl), QIODevice::ReadOnly);
}

void FfmpegRtspDecoder::stop()
{
    m_stopRequested = true;
    m_stdoutBuffer.clear();

    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(800)) {
            m_process->kill();
            m_process->waitForFinished(800);
        }
    }

    emit stopped();
}

void FfmpegRtspDecoder::onReadyReadStandardOutput()
{
    if (m_frameBytes <= 0) {
        m_process->readAllStandardOutput();
        return;
    }

    m_stdoutBuffer += m_process->readAllStandardOutput();
    const int completeFrames = m_stdoutBuffer.size() / m_frameBytes;
    if (completeFrames <= 0) {
        return;
    }

    const int latestOffset = (completeFrames - 1) * m_frameBytes;
    const QByteArray latestFrame = m_stdoutBuffer.mid(latestOffset, m_frameBytes);
    m_stdoutBuffer.remove(0, completeFrames * m_frameBytes);

    QImage frame(reinterpret_cast<const uchar *>(latestFrame.constData()),
                 m_width,
                 m_height,
                 m_width * kBytesPerPixel,
                 QImage::Format_RGB888);
    emit frameReady(frame.copy());
}

void FfmpegRtspDecoder::onReadyReadStandardError()
{
    m_stderrBuffer += QString::fromLocal8Bit(m_process->readAllStandardError());
    if (m_stderrBuffer.size() > kStderrLimit) {
        m_stderrBuffer = m_stderrBuffer.right(kStderrLimit);
    }
}

void FfmpegRtspDecoder::onProcessError(QProcess::ProcessError error)
{
    if (m_stopRequested) {
        return;
    }

    if (error == QProcess::FailedToStart) {
        emit failed(QStringLiteral("无法启动 ffmpeg，请确认 ffmpeg 在 PATH 中，或设置 HOSTCOMPUTER_FFMPEG_PATH"));
        return;
    }

    emit failed(QStringLiteral("ffmpeg 进程错误: %1").arg(m_process->errorString()));
}

void FfmpegRtspDecoder::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_stdoutBuffer.clear();

    if (m_stopRequested) {
        return;
    }

    const QString status = exitStatus == QProcess::CrashExit
        ? QStringLiteral("崩溃")
        : QStringLiteral("退出");
    emit failed(QStringLiteral("ffmpeg %1，code=%2%3")
                    .arg(status)
                    .arg(exitCode)
                    .arg(stderrTail()));
}

QString FfmpegRtspDecoder::ffmpegProgram() const
{
    const QString configured = qEnvironmentVariable("HOSTCOMPUTER_FFMPEG_PATH");
    if (!configured.trimmed().isEmpty()) {
        return configured.trimmed();
    }

    const QString appDir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    const QString bundled = QDir(appDir).filePath(QStringLiteral("ffmpeg.exe"));
#else
    const QString bundled = QDir(appDir).filePath(QStringLiteral("ffmpeg"));
#endif
    if (QFileInfo::exists(bundled)) {
        return bundled;
    }

    const QString inPath = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    return inPath.isEmpty() ? QStringLiteral("ffmpeg") : inPath;
}

QStringList FfmpegRtspDecoder::ffmpegArguments(const QString &rtspUrl) const
{
    Q_UNUSED(m_fps)

    return {
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"),
        QStringLiteral("warning"),
        QStringLiteral("-rtsp_transport"),
        QStringLiteral("tcp"),
        QStringLiteral("-fflags"),
        QStringLiteral("nobuffer"),
        QStringLiteral("-flags"),
        QStringLiteral("low_delay"),
        QStringLiteral("-analyzeduration"),
        QStringLiteral("0"),
        QStringLiteral("-probesize"),
        QStringLiteral("32768"),
        QStringLiteral("-i"),
        rtspUrl,
        QStringLiteral("-an"),
        QStringLiteral("-vf"),
        QStringLiteral("scale=%1:%2").arg(m_width).arg(m_height),
        QStringLiteral("-f"),
        QStringLiteral("rawvideo"),
        QStringLiteral("-pix_fmt"),
        QStringLiteral("rgb24"),
        QStringLiteral("-")
    };
}

QString FfmpegRtspDecoder::stderrTail() const
{
    const QString tail = m_stderrBuffer.trimmed();
    if (tail.isEmpty()) {
        return QString();
    }
    return QStringLiteral(": %1").arg(tail.right(500));
}
