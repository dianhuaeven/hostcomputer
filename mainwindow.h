#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 状态更新接口
    void updateConnectionStatus(bool connected);
    void updateHeartbeatStatus(bool online);
    void updateFPS(int fps);
    void updateCPUUsage(int cpu, int gpu);
    void updateMotorMode(const QString& mode);
    void addCommand(const QString& command);
    void addError(const QString& error);
    void updateCarAttitude(double roll, double pitch, double yaw);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_action_connect_triggered();
    void on_action_disconnect_triggered();
    void on_action_exit_triggered();
    void on_action_fullscreen_triggered();
    void on_action_reset_layout_triggered();
    void on_action_about_triggered();

    void on_btn_clear_commands_clicked();
    void on_btn_clear_errors_clicked();

    void updateSystemStatus();  // 定时更新系统状态

private:
    void setupConnections();
    void setupStatusBar();
    void updateConnectionDisplay();
    void updateHeartbeatDisplay();
    void formatAndAddCommand(const QString& command);
    void formatAndAddError(const QString& error);
    QString getCurrentTimestamp() const;

private:
    Ui::MainWindow *ui;

    // 状态管理
    bool m_isConnected = false;
    bool m_heartbeatOnline = false;
    int m_currentFPS = 0;
    int m_cpuUsage = 0;
    int m_gpuUsage = 0;
    QString m_motorMode = "待机";
    int m_errorCount = 0;

    // 定时器
    QTimer* m_statusTimer;

    // 姿态数据
    double m_roll = 0.0;
    double m_pitch = 0.0;
    double m_yaw = 0.0;
};

#endif // MAINWINDOW_H
