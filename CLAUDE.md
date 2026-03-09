# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个基于Qt6的C++桌面应用程序（上位机），用于机器人电机控制、实时监控和多摄像头显示。

**系统架构**:
```
┌─────────────────┐         TCP/JSON          ┌─────────────────┐
│   Windows PC    │ ◄────────────────────────► │  Intel NUC      │
│   (上位机)       │         Port 9090           │  (下位机)        │
│   Qt6 App       │                            │  Ubuntu 20.04   │
└────────┬────────┘                            │  ROS1           │
         │                                     └────────┬────────┘
         │                                              │
         │         USB CDC / 串口                       │
         │ ◄──────────────────────────────────────────► │
         │           52字节协议帧                       │
         │                                              │
         │         UDP MJPEG                           │
         │ ◄──────────────────────────────────────────► │
         │           Port 5000                          │
         │                                              │
         ▼                                              ▼
    ┌─────────┐                                    ┌─────────┐
    │ 6路视频 │                                    │ 摄像头  │
    │ 显示    │                                    │ 模组    │
    └─────────┘                                    └─────────┘
```

**设备说明**:
- **上位机**: Windows PC 运行本Qt应用程序
- **下位机**: Intel NUC 运行 Ubuntu 20.04 + ROS1，负责电机控制和传感器数据处理
- **ESP32**: 额外模块，通过USB CDC与上位机通信，用于特定功能（如关节数据采集）

项目采用四层分层架构设计，使用CMake构建系统，目标平台为Windows。

## 构建命令

### Windows平台构建
```bash
# 创建构建目录
mkdir build
cd build

# 配置CMake项目（根据环境选择生成器）
# MinGW环境
cmake .. -G "MinGW Makefiles"
# 或使用Ninja（如果已安装）
cmake .. -G "Ninja"

# 编译项目
# MinGW
mingw32-make
# 或使用Ninja
ninja

# 运行程序
./hostcomputer.exe
```

### 开发调试
```bash
# 增量编译（build目录下）
mingw32-make  # 或 ninja

# 清理构建产物
mingw32-make clean  # 或 ninja clean

# 重新配置CMake
cmake .. -G "MinGW Makefiles"
```

## 分层架构

项目严格遵循四层架构，每层只能调用下层，不能跨层调用：

```
┌─────────────────┐
│   MainWindow    │ ← UI层，主窗口
│   Controller    │ ← 业务逻辑层（src/controller/）
├─────────────────┤
│ Communication   │ ← 通信层（src/communication/）
├─────────────────┤
│     Parser      │ ← 解析层（src/parser/）
├─────────────────┤
│      Model      │ ← 模型层（src/model/）
└─────────────────┘
```

### 1. Communication层 (src/communication/)
**职责**: TCP、串口、UDP通信管理

**核心组件**:
- `ROS1TcpClient`: 与Intel NUC (Ubuntu ROS1) 的TCP通信，JSON格式数据交换 ⭐主要通道
- `SerialPortManager`: 串口设备管理、ESP32 USB CDC通信
- `UdpVideoReceiver`: UDP MJPEG视频流接收
- `SharedStructs.h`: 共享数据结构定义

**关键数据结构** (SharedStructs.h):
```cpp
namespace Communication {
    struct JointState {
        int16_t position;      // 关节位置
        int16_t current;       // 关节电流
    };

    struct ESP32State {
        std::array<JointState, 6> joints;  // 6个关节数据
        int16_t executor_position;
        int16_t executor_torque;
        uint8_t executor_flags;
        uint8_t reserved;
    };

    struct Command {
        std::array<int16_t, 6> target_position;
        std::array<int16_t, 6> target_torque;
        int16_t executor_position;
        int16_t executor_torque;
        uint8_t command_flags;
        uint8_t reserved;
    };
}
```

**规则**:
- ✅ 管理设备连接和数据传输
- ✅ 发送和接收原始数据帧
- ❌ 不包含业务逻辑
- ✅ 只发出raw frame（原始数据帧）

### 2. Parser层 (src/parser/)
**职责**: 将原始数据(QByteArray)解析为结构化数据

**核心组件**:
- `ProtocolParser`: 52字节USB CDC协议帧编解码
- `motor_state.h`: MotorState和JointState结构体定义

**协议格式**:
- 帧结构: Header(0xAA) + Version + Length + Payload(48B) + Checksum
- 帧长度: 52字节固定
- 关节数量: 6个关节

**规则**:
- ✅ 处理协议解析和数据转换
- ✅ 数据校验和完整性检查
- ❌ 不发送命令
- ❌ 不访问UI

### 3. Model层 (src/model/)
**职责**: UI数据模型，使用QObject+Q_PROPERTY实现数据绑定

**核心组件**:
- `MotorModel`: 电机数据模型

**规则**:
- ✅ 只用于UI数据绑定
- ❌ 不包含业务逻辑
- ❌ 不包含通信逻辑

### 4. Controller层 (src/controller/)
**职责**: 业务逻辑处理，协调其他各层

**核心组件**:
- `Controller`: 主控制器类
- `RealCameraManager`: 真实摄  头管理器
- `VideoWidget`: 视频显示控件

**摄像头管理系统**:
```cpp
class RealCameraManager : public QObject {
    bool startCamera(int cameraId, VideoWidget* displayWidget);
    void stopCamera(int cameraId);
    void stopAllCameras();
    QStringList getAvailableCameras() const;

signals:
    void cameraStarted(int cameraId, const QString& cameraName);
    void cameraStopped(int cameraId);
    void cameraError(int cameraId, const QString& error);
};
```

**规则**:
- ✅ 负责业务逻辑处理
- ✅ 协调各层之间的交互
- ❌ 不处理底层通信细节（交给Communication层）
- ❌ 不直接操作QByteArray（交给Parser层）

## 数据流架构

### 主通讯通道 (TCP + JSON) - 与Intel NUC下位机

```
Intel NUC (下位机)                    Windows PC (上位机)
Ubuntu 20.04 + ROS1                   Qt6 Application
     │                                       │
     │  ① 发送状态数据 (JSON)                │
     ├─────────────────────────────────────► │
     │      TCP Port 9090                    │
     │   {                                   │
     │     "type": "motor_state",            │
     │     "data": {                         │
     │       "joints": [...]                 │
     │     }                                 │
     │   }                                   │
     │                                       │
     │                              ② ROS1TcpClient接收
     │                                       │
     │                              ③ JSON解析 → ESP32State
     │                                       │
     │                              ④ emit motorStateReceived
     │                                       │
     │                              ⑤ MainWindow更新UI
     │
     │  ⑥ 发送控制命令 (JSON)              │
     │  ◄─────────────────────────────────────┤
     │      用户操作触发                       │
```

### 辅助通道 (USB CDC) - 与ESP32模块

```
ESP32模块                                Windows PC (上位机)
     │                                       │
     │  ① 52字节协议帧                       │
     ├─────────────────────────────────────► │
     │      USB CDC / 串口                   │
     │   Header(0xAA) + Version + ...        │
     │                                       │
     │                              ② SerialPortManager接收
     │                                       │
     │                              ③ ProtocolParser解析
     │                                       │
     │                              ④ emit esp32StateReceived
     │                                       │
     │                              ⑤ MainWindow显示关节数据
```

### 视频流通道 (UDP MJPEG) - 来自NUC/摄像头

```
Intel NUC / 摄像头                    Windows PC (上位机)
     │                                       │
     │  ① JPEG帧分包发送                     │
     ├─────────────────────────────────────► │
     │      UDP Port 5000                    │
     │   Header + Data分片 + End             │
     │                                       │
     │                              ② UdpVideoReceiver接收
     │                                       │
     │                              ③ 分片重组
     │                                       │
     │                              ④ QImage解码
     │                                       │
     │                              ⑤ VideoWidget显示
```

## 技术栈

- **框架**: Qt6.8.3 (兼容Qt5)
- **C++标准**: C++20
- **构建系统**: CMake 3.16+
- **Qt模块**: Core, Widgets, Network, SerialPort, Multimedia, MultimediaWidgets
- **可选依赖**: OpenCV 4.x (摄像头备用方案)

## Qt模块依赖

### 必需模块
- Qt6::Core (核心功能)
- Qt6::Widgets (传统Widget UI)
- Qt6::Network (网络通信)
- Qt6::SerialPort (串口通信)
- Qt6::Multimedia (摄像头支持)
- Qt6::MultimediaWidgets (视频显示控件)

### 可选模块
- Qt6::OpenGL + Qt6::OpenGLWidgets (3D渲染)
- Qt6::Charts (图表显示)

## 关键设计原则

1. **严格遵循分层架构**: 每层只能调用下层，不能跨层调用
2. **使用Qt信号槽机制**: 实现异步通信和组件解耦
3. **模块化设计**: 每层作为独立的静态库
4. **实时性考虑**: 作为上位机应用，重点关注通信实时性和数据处理的稳定性

## 命名空间

- `Communication::`: 通信层相关类和数据结构
- `Parser::`: 解析层相关类

## 已实现功能

### 通讯功能
1. **TCP JSON通信** ⭐: 与Intel NUC (Ubuntu ROS1) 主通讯通道
   - 接收电机状态数据
   - 发送控制命令
   - 自动重连机制
2. **串口通信**: ESP32模块USB CDC通信
3. **UDP视频接收**: MJPEG视频流，支持6路摄像头

### UI功能
1. **多摄像头管理**: 最多6路USB摄像头并发显示
2. **实时数据显示**: 关节位置、电流、执行器状态
3. **系统状态监控**: 通讯状态、统计信息

## 通讯协议详解

### TCP JSON协议 (与Intel NUC)

**接收状态数据格式**:
```json
{
  "type": "motor_state",
  "timestamp": 1234567890,
  "data": {
    "joints": [
      {"id": 0, "position": 1000, "current": 500},
      {"id": 1, "position": 2000, "current": 600},
      ...
    ],
    "executor": {
      "position": 500,
      "torque": 1000
    }
  }
}
```

**发送控制命令格式**:
```json
{
  "type": "motor_command",
  "timestamp": 1234567890,
  "data": {
    "joints": [
      {"id": 0, "target_position": 1500, "target_torque": 800},
      ...
    ],
    "executor": {
      "target_position": 600,
      "target_torque": 1200
    }
  }
}
```

### USB CDC协议 (与ESP32)

**52字节固定帧结构**:
```
Offset  Size    Field           Description
-----  -----   ------           -----------
0      1B      Header          0xAA 帧头
1      1B      Version         0x01 协议版本
2      1B      Length          52 帧长度
3-26   24B     joints[6]       6关节×(位置+电流)
27-28  2B      executor_pos    执行器位置
29-30  2B      executor_torque 执行器扭矩
31     1B      flags           标志位
32     1B      reserved        保留
33-50  18B     padding         填充0
51     1B      Checksum        校验和
```

### UDP MJPEG协议 (视频流)

**UDP包结构**:
```
Offset  Size    Field           Description
-----  -----   ------           -----------
0      1B      magic           0xAA
1      1B      frameType       01=Header 02=Data 03=End
2-3    2B      frameId         帧序号
4-5    2B      packetIndex     分片序号
6-7    2B      totalPackets    总分片数
8-11   4B      payloadSize     JPEG大小
12-15  4B      timestamp       时间戳
16     1B      cameraId        摄像头ID(0-5)
17-19  3B      reserved        保留
20+    ~1400B  payload         JPEG数据分片
```

---

## 对话要求
对我的称呼为爸爸，每次对话必须以喵字结尾