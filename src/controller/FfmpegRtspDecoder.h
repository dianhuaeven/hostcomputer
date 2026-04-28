#ifndef FFMPEGRTSPDECODER_H
#define FFMPEGRTSPDECODER_H

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QProcess>
#include <QString>

class FfmpegRtspDecoder : public QObject
{
    Q_OBJECT

public:
    explicit FfmpegRtspDecoder(QObject *parent = nullptr);
    ~FfmpegRtspDecoder() override;

    bool isRunning() const;
    void start(const QString &rtspUrl, int width, int height, int fps);
    void stop();

signals:
    void started();
    void stopped();
    void frameReady(const QImage &frame);
    void failed(const QString &message);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString ffmpegProgram() const;
    QStringList ffmpegArguments(const QString &rtspUrl) const;
    QString stderrTail() const;

    QProcess *m_process = nullptr;
    QByteArray m_stdoutBuffer;
    QString m_stderrBuffer;
    int m_width = 0;
    int m_height = 0;
    int m_fps = 0;
    int m_frameBytes = 0;
    bool m_stopRequested = false;
};

#endif // FFMPEGRTSPDECODER_H
