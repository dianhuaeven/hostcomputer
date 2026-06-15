# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

基于Qt6的C++桌面上位机应用，用于机器人电机控制、实时监控和多摄像头显示。通过TCP/JSON与Intel NUC下位机（Ubuntu 20.04 + ROS1）通信，支持RTSP视频流、手柄/键盘控制、IMU姿态显示、CO2传感器等功能。

```
Windows PC (上位机 Qt6)  ◄──TCP/JSON Port 9090──►  Intel NUC (下位机 ROS1)
                         ◄──RTSP视频流──────────►  摄像头模组 (2×3 网格 6 格，当前启用 5 路)
```

## 构建命令

```bash
# 在项目根目录下（使用Qt Creator配置的构建目录）
cd build/Desktop_Qt_6_8_3_MinGW_64_bit-Debug

# 增量编译
cmake --build .
# 或直接
ninja

# 清理重建
ninja clean && ninja

# 从头配置（如修改了CMakeLists.txt）
cmake ../.. -G "Ninja"
ninja
```

**重要**：项目使用 `-O0` 全局禁用优化，绕过 GCC 13.1 汇编器 Segmentation fault 的已知bug。不要添加优化标志。

## 分层架构

- **UI层**（根目录 `mainwindow.h/cpp/ui`）：MainWindow 组装所有组件，处理键盘/手柄事件分发、菜单与状态栏；嵌入 QML（`RobotView.qml` + `RobotViewModel`）渲染 3D 姿态
- **Controller 层**（`src/controller/`）：业务逻辑层 + 自定义 UI 控件 + 输入采集
  - 业务/输入：`Controller`、`KeyboardController`、`HandleKey`（SDL3 手柄，Windows 下 XInput fallback）
  - 视频栈：`CameraGridWidget`（2×3 网格容器）→ `RtspPlayerWidget`（单路单元）→ `FfmpegRtspDecoder`（FFmpeg 子进程解码）+ `VideoFrameWidget`（帧绘制）；`DisplayLayoutManager`（网格布局）
  - 状态显示：`MotorRuntimeCarouselWidget`、`RobotAttitudeWidget`、`ControlPanelWidget`、`TelemetryPanelWidget`、`GamepadDisplayWidget`、`CO2DisplayWidget`（⚠️ 死代码）
- **Communication 层**（`src/communication/`）：仅 TCP/JSON 收发，不含业务逻辑 —— `ROS1TcpClient`（连接/重连/心跳/粘包/ACK）+ `HostProtocol`（上行帧封装）+ `SharedStructs`
- **Utils 层**（`src/utils/`）：`Logger` + `ErrorHandler`，被所有层依赖

每层编译为独立静态库（`utils_layer` / `communication_layer` / `controller_layer`）。依赖方向：上层 → 下层，通过Qt信号槽解耦。

> ⚠️ 架构现状：`Controller` 当前退化为透传层，键盘/手柄输入未在其内部合并，大量业务逻辑堆积在 MainWindow。详见 `docs/已知问题与技术债务.md` 系统性问题 #1。

### 层级规则
- **Communication层**：只负责TCP连接和原始JSON收发，不含业务逻辑。通过signal将数据向上传递
- **Controller层**：协调通信层，提供统一业务接口给UI层，也包含自定义UI控件
- **Utils层**：Logger日志和ErrorHandler，被所有层依赖
- **UI层**（根目录）：MainWindow组装所有组件，处理键盘/手柄事件分发

### 关键数据流

下位机状态 → `ROS1TcpClient`(TCP JSON接收) → `Controller`(信号转发) → `MainWindow`(更新UI)

用户操作 → `MainWindow`(事件捕获) → `KeyboardController`/`HandleKey` → `Controller`(命令封装) → `ROS1TcpClient`(JSON发送) → 下位机

## 核心组件

### 通信 (src/communication/)
- **`ROS1TcpClient`**：异步TCP客户端，JSON格式数据交换，自动重连，心跳检测。主要信号：`motorStateReceived`、`jointRuntimeStatesReceived`（电机生命状态，驱动 MotorRuntimeCarousel）、`jointDataReceived`、`imuDataReceived`、`co2DataReceived`、`cameraInfoReceived`、`systemStatusReceived`、`protocolMessageReceived`（hello/capabilities/ack/service_call_result 等透传）、`heartbeatChanged`、`statsUpdated`
- **`HostProtocol`**：无状态协议封装层，构造上行帧（operator_input/heartbeat/sync_request/camera_list_request/command 的信封+业务字段）
- **`SharedStructs.h`**：定义`Communication::MotorState`、`JointRuntimeState`、`OperatorInputState`、`Command`等跨层数据结构

### 控制 (src/controller/)
- **`Controller`**：业务逻辑层入口。⚠️ 当前退化为透传壳，主要提供 `sendOperatorInput`/`sendEmergencyStop`/`sendSystemCommand`/`requestCameraList`/`requestBridgeSync`；`sendMotorCommand`/`sendJointControl`/`sendEndEffectorControl`/`sendControlCommand` 已实现但 MainWindow 未启用
- **`KeyboardController`**：采集键盘输入并输出协议层按键名（车体/机械臂双模式），10Hz 定时发出按键快照
- **`HandleKey`**：SDL3 Gamepad手柄驱动（受 `USE_SDL3_GAMEPAD` 宏控制），Windows下保留XInput fallback，50Hz 轮询手柄状态并发出`ControllerState`信号
- **`CameraGridWidget`**：2×3 视频网格容器（6 格，当前启用 5 路），管理网格/单路聚焦切换，内含多个 `RtspPlayerWidget`
- **`RtspPlayerWidget`**：单路 RTSP 播放单元，组合 `FfmpegRtspDecoder` + `VideoFrameWidget`，支持右键鱼眼校正
- **`FfmpegRtspDecoder`** / **`VideoFrameWidget`**：FFmpeg 子进程低缓冲拉流解码（rawvideo）+ 帧绘制
- **`DisplayLayoutManager`**：2×3 QGridLayout 薄封装
- **`MotorRuntimeCarouselWidget`**：电机生命状态轮播显示（驱动自 `jointRuntimeStatesReceived`）
- **`RobotAttitudeWidget`** + **`RobotViewModel`**：QML 与 C++ 桥接，3D 姿态渲染（Roll/Pitch/Yaw + 四腿角度）
- **`ControlPanelWidget`** / **`TelemetryPanelWidget`** / **`GamepadDisplayWidget`**：控制/遥测/手柄状态面板

### 控制模式
- **Vehicle模式**：键盘WASD/手柄摇杆控制车体运动（linearX/Y, angularZ）
- **Arm模式**：控制机械臂关节和末端执行器

## 技术栈

- Qt 6.8.3 (MinGW 64-bit), C++20, CMake 3.16+
- Qt模块：Core, Widgets, Network, Quick, QuickWidgets
- 运行时工具：FFmpeg 可执行文件用于上位机 RTSP 低延迟播放、下位机侧 ROS 图像推流
- 开发机 FFmpeg 安装说明见 `docs/开发归档/FFmpeg开发环境安装指南.md`
- Windows XInput（手柄支持）
- QML用于3D机器人姿态渲染（`resources/qml/RobotView.qml`）

## 通讯协议

### TCP JSON协议 (Port 9090)

接收：`motor_state`（关节位置/电流）、`joint_runtime_states`（电机生命状态）、`co2_data`、`imu_data`、`camera_info`（含RTSP URL）、`system_status`、`hello`/`capabilities`（握手）、`ack`/`service_call_result`、`emergency_state` 等

发送：`operator_input`（高频键盘/手柄输入快照，不逐条 ACK）、`emergency_stop` / `system_command`（关键命令，必须 ACK），以及保留的低频机械臂/电机控制帧（`motor_command`/`joint_control`/`cartesian_control`/`control_command`，当前 MainWindow 未启用发送）。

摄像头信息推送触发RTSP自动播放：`camera_id` 0-5 对应 2×3 网格全部 6 格（`MAX_CAMERA_COUNT=6`，当前配置启用 5 路，第 6 路 backcam reserved），`online: true` 时启动播放。

## 开发注意事项

- 修改CMakeLists.txt后务必验证编译通过，不要随意添加/删除依赖
- GCC 13.1已知bug：不要对`mainwindow.cpp`、`handlekey.cpp`、`ErrorHandler.cpp`启用优化
- 代码修改后立即编译验证，不要批量修改后再编译
- UI布局修改前先确认布局方案，避免反复迭代
- 下位机（ros1_bridge + ROS1）无实际机器人硬件，无法测试验证；修复优先以上位机可独立验证的问题为主。已知问题见 `docs/已知问题与技术债务.md`

## 测试与调试

```bash
# 模拟下位机ROS1服务器（用于无硬件调试）
python scripts/mock_ros1_server.py
```

## 对话要求
对我的称呼为主人，每次对话必须以喵字结尾
