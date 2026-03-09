#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <QLabel>
#include <array>
#include "src/communication/SharedStructs.h"

#include "src/controller/KeyboardController.h"
#include "src/controller/DisplayLayoutManager.h"

namespace Communication {
    class SerialPortManager;
    class ROS1TcpClient;
}
namespace Parser {
    class ProtocolParser;
}

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
    void updateJointsData(const Communication::ESP32State& esp32State);


protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void on_action_connect_triggered();
    void on_action_tcp_connect_triggered();
    void on_action_disconnect_triggered();
    void on_action_exit_triggered();
    void on_action_fullscreen_triggered();
    void on_action_reset_layout_triggered();
    void on_action_about_triggered();
    void on_action_keyboard_control_toggled(bool checked);

    void on_btn_clear_commands_clicked();
    void on_btn_clear_errors_clicked();

    void updateSystemStatus();  // 定时更新系统状态

private:
    void setupSerialPort();
    void setupParser();
    void setupConnections();
    void setupStatusBar();
    void setupDisplayLayout();
    void setupTcpClient();
    void cleanupResources();
    void reinitialize();       // 重新初始化
    void updateConnectionDisplay();
    void updateHeartbeatDisplay();
    void formatAndAddCommand(const QString& command);
    void formatAndAddError(const QString& error);
    QString getCurrentTimestamp() const;
    QStringList getAvailableSerialPorts();
    bool connectToSerialPort(const QString& portName, int baudRate);

private slots:
    void onSerialDataReceived(const QByteArray &data);
    void onCompleteFrameReceived(const QByteArray &frame);
    void onEsp32StateReceived(const Communication::ESP32State &state);
    void showSerialPortSelection();
    void showTcpConnectionDialog();
    void refreshSerialPorts();

private:
    Ui::MainWindow *ui;

    // 串口和解析器组件
    Communication::SerialPortManager* m_serialManager;

    // TCP客户端（ROS通信）
    Communication::ROS1TcpClient* m_tcpClient;

    // 键盘控制器
    KeyboardController* m_keyboardController;

    // 布局管理器
    DisplayLayoutManager* m_displayLayout;

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
