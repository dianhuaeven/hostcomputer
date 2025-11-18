#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QCloseEvent>
#include <QMessageBox>
#include <QStatusBar>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 设置窗口属性
    setWindowTitle("电机控制系统上位机 v1.0");
    resize(1920, 1080);
    setMinimumSize(1600, 900);

    // 初始化定时器
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updateSystemStatus);
    m_statusTimer->start(1000); // 每秒更新一次状态

    // 设置连接和状态栏
    setupConnections();
    setupStatusBar();

    // 初始化UI状态
    updateConnectionDisplay();
    updateHeartbeatDisplay();

    qDebug() << "主窗口初始化完成";
}

MainWindow::~MainWindow()
{
    if (m_statusTimer) {
        m_statusTimer->stop();
    }
    delete ui;
}

void MainWindow::setupConnections()
{
    // 按钮连接已在UI文件中自动处理，这里可以添加额外的连接
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("系统就绪");
}

void MainWindow::updateConnectionStatus(bool connected)
{
    m_isConnected = connected;
    updateConnectionDisplay();

    if (connected) {
        statusBar()->showMessage("设备已连接");
        addCommand("[系统] 设备连接成功");
    } else {
        statusBar()->showMessage("设备已断开");
        addCommand("[系统] 设备连接断开");
    }
}

void MainWindow::updateHeartbeatStatus(bool online)
{
    m_heartbeatOnline = online;
    updateHeartbeatDisplay();

    if (!online && m_isConnected) {
        addError("[警告] 下位机心跳丢失");
    }
}

void MainWindow::updateFPS(int fps)
{
    m_currentFPS = fps;
    ui->label_fps_value->setText(QString("%1 FPS").arg(fps));
}

void MainWindow::updateCPUUsage(int cpu, int gpu)
{
    m_cpuUsage = cpu;
    m_gpuUsage = gpu;
    ui->label_cpu_value->setText(QString("%1% / %2%").arg(cpu).arg(gpu));
}

void MainWindow::updateMotorMode(const QString& mode)
{
    m_motorMode = mode;
    ui->label_mode_value->setText(mode);
    addCommand(QString("[系统] 电机模式切换: %1").arg(mode));
}

void MainWindow::addCommand(const QString& command)
{
    formatAndAddCommand(command);
}

void MainWindow::addError(const QString& error)
{
    m_errorCount++;
    formatAndAddError(error);
    ui->label_error_count->setText(QString("错误: %1").arg(m_errorCount));
}

void MainWindow::updateCarAttitude(double roll, double pitch, double yaw)
{
    m_roll = roll;
    m_pitch = pitch;
    m_yaw = yaw;

    ui->label_roll->setText(QString("Roll: %1°").arg(roll, 0, 'f', 1));
    ui->label_pitch->setText(QString("Pitch: %1°").arg(pitch, 0, 'f', 1));
    ui->label_yaw->setText(QString("Yaw: %1°").arg(yaw, 0, 'f', 1));

    // TODO: 这里可以更新3D姿态模型显示
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认退出",
        "确定要退出电机控制系统吗？",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (m_isConnected) {
            // 发送断开连接命令
            updateConnectionStatus(false);
        }
        event->accept();
    } else {
        event->ignore();
    }
}

// 菜单槽函数
void MainWindow::on_action_connect_triggered()
{
    // TODO: 实现连接逻辑
    updateConnectionStatus(true);
    addCommand("[操作] 用户点击连接设备");
}

void MainWindow::on_action_disconnect_triggered()
{
    // TODO: 实现断开连接逻辑
    updateConnectionStatus(false);
    addCommand("[操作] 用户点击断开连接");
}

void MainWindow::on_action_exit_triggered()
{
    close();
}

void MainWindow::on_action_fullscreen_triggered()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::on_action_reset_layout_triggered()
{
    // TODO: 重置窗口布局
    QMessageBox::information(this, "提示", "布局重置功能开发中...");
}

void MainWindow::on_action_about_triggered()
{
    QMessageBox::about(this, "关于",
        "电机控制系统上位机 v1.0\n\n"
        "基于Qt6开发的电机控制和监控系统\n"
        "支持多相机显示、姿态监控、指令管理等功能\n\n"
        "开发团队: AI Assistant\n"
        "技术支持: Qt6.8 + CMake");
}

void MainWindow::on_btn_clear_commands_clicked()
{
    ui->text_commands->clear();
    addCommand("[系统] 指令记录已清空");
}

void MainWindow::on_btn_clear_errors_clicked()
{
    ui->text_errors->clear();
    m_errorCount = 0;
    ui->label_error_count->setText("错误: 0");
    addCommand("[系统] 错误记录已清空");
}

void MainWindow::updateSystemStatus()
{
    // 模拟系统状态更新
    // TODO: 实现真实的CPU/GPU监控
    static int counter = 0;
    counter++;

    // 模拟FPS变化
    if (counter % 3 == 0 && m_isConnected) {
        int simulatedFPS = 25 + (counter % 10);
        updateFPS(simulatedFPS);
    }

    // 模拟CPU/GPU使用率
    if (counter % 5 == 0) {
        int simulatedCPU = 20 + (counter % 30);
        int simulatedGPU = 15 + (counter % 25);
        updateCPUUsage(simulatedCPU, simulatedGPU);
    }
}

void MainWindow::updateConnectionDisplay()
{
    QString statusText = m_isConnected ? "已连接" : "断开";
    QString styleSheet = m_isConnected ?
        "color: green; font-weight: bold;" :
        "color: red; font-weight: bold;";

    ui->label_connection_value->setText(statusText);
    ui->label_connection_value->setStyleSheet(styleSheet);
}

void MainWindow::updateHeartbeatDisplay()
{
    QString statusText = m_heartbeatOnline ? "在线" : "丢失";
    QString styleSheet = m_heartbeatOnline ?
        "color: green; font-weight: bold;" :
        "color: red; font-weight: bold;";

    ui->label_heartbeat_value->setText(statusText);
    ui->label_heartbeat_value->setStyleSheet(styleSheet);
}

void MainWindow::formatAndAddCommand(const QString& command)
{
    QString timestamp = getCurrentTimestamp();
    QString formattedCommand = QString("[%1] %2").arg(timestamp, command);

    QTextCursor cursor = ui->text_commands->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(formattedCommand + "\n");

    // 自动滚动到底部
    if (ui->checkbox_auto_scroll->isChecked()) {
        ui->text_commands->setTextCursor(cursor);
        ui->text_commands->ensureCursorVisible();
    }

    // 限制显示行数，避免内存占用过多
    QStringList lines = ui->text_commands->toPlainText().split('\n');
    if (lines.size() > 1000) {
        lines = lines.mid(-500); // 保留最近500行
        ui->text_commands->setText(lines.join('\n'));
    }
}

void MainWindow::formatAndAddError(const QString& error)
{
    QString timestamp = getCurrentTimestamp();
    QString formattedError = QString("[%1] %2").arg(timestamp, error);

    QTextCursor cursor = ui->text_errors->textCursor();
    cursor.movePosition(QTextCursor::End);

    // 设置错误文本颜色为红色
    QTextCharFormat format;
    format.setForeground(QColor(200, 0, 0));
    cursor.setCharFormat(format);
    cursor.insertText(formattedError + "\n");

    // 自动滚动到底部
    ui->text_errors->setTextCursor(cursor);
    ui->text_errors->ensureCursorVisible();

    // 限制显示行数
    QStringList lines = ui->text_errors->toPlainText().split('\n');
    if (lines.size() > 500) {
        lines = lines.mid(-200); // 保留最近200行错误
        ui->text_errors->setText(lines.join('\n'));
    }
}

QString MainWindow::getCurrentTimestamp() const
{
    return QDateTime::currentDateTime().toString("hh:mm:ss");
}
