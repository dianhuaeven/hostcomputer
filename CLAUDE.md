# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个基于Qt6的C++桌面应用程序，用于电机控制和通信。项目采用分层架构设计，使用CMake构建系统，目标平台为Windows。

## 构建命令

### 基础构建
```bash
# 创建构建目录
mkdir build
cd build

# 配置CMake项目
cmake .. -G "Ninja"

# 编译项目
ninja

# 运行程序
./hostcomputer.exe
```

### 开发调试
```bash
# 增量编译（build目录下）
ninja

# 清理构建产物
ninja clean

# 重新配置CMake
cmake .. -G "Ninja"
```

## 项目架构

项目采用四层分层架构，按职责分离��

### 1. Model层 (src/model/)
- **职责**: UI数据模型，使用QObject+Q_PROPERTY实现数据绑定
- **核心组件**: `MotorModel`类
- **依赖**: Qt6::Core

### 2. Parser层 (src/parser/)
- **职责**: 将原始数据(QByteArray)解析为结构化数据(MotorState)
- **核心组件**: `Parser`类、`MotorState`结构体
- **依赖**: Qt6::Core

### 3. Communication层 (src/communication/)
- **职责**: CAN总线、串口、TCP通信管理
- **核心组件**: `CANManager`类
- **依赖**: Qt6::Core, Qt6::Network, Qt6::SerialPort

### 4. Controller层 (src/controller/)
- **职责**: 业务逻辑处理，协调其他各层
- **依赖**: 其他所有层级

## 技术栈

- **框架**: Qt6.8.3
- **C++标准**: C++20
- **构建系统**: CMake 3.16+ + Ninja
- **UI技术**: Qt Widgets + Qt Quick (QML)
- **通信模块**: CAN、串口、TCP
- **图形渲染**: OpenGL
- **数据可视化**: Qt Charts

## Qt模块依赖

项目配置了以下Qt6模块：
- Qt6::Core (核心功能)
- Qt6::Widgets (传统Widget UI)
- Qt6::Quick + Qt6::QuickWidgets (QML支持)
- Qt6::Network (网络通信)
- Qt6::SerialPort (串口通信)
- Qt6::OpenGL + Qt6::OpenGLWidgets (3D渲染)
- Qt6::Charts (图表显示)

## 开发优先级

建议按以下顺序实现各层：

1. **Model层**: 首先实现数据结构和MotorModel
2. **Parser层**: 基础数据解析功能
3. **Communication层**: CAN通信管理
4. **Controller层**: 业务逻辑和UI界面

## 关键设计原则

1. **严格遵循分层架构**: 每层只能调用下层，不能跨层调用
2. **使用Qt信号槽机制**: 实现异步通信和组件解耦
3. **模块化设计**: 每层作为独立的静态库
4. **实时性考虑**: 作为上位机应用，重点关注通信实时性和数据处理的稳定性

## 项目结构

```
hostcomputer/
├── src/
│   ├── controller/      # 业务逻辑层
│   ├── communication/   # 通信层
│   ├── parser/          # 协议解析层
│   └── model/           # 数据模型层
├── resources/           # 资源文件
├── build/              # 构建输出
├── CMakeLists.txt      # 主CMake配置
├── main.cpp           # 程序入口
├── mainwindow.h/.cpp  # 主窗口
└── mainwindow.ui      # UI设计文件
```

## 注意事项

- 项目当前处于架构设计完成阶段，业务逻辑实现尚未开始
- 所有src/子目录下的源文件(.h/.cpp)都已创建但内容为空
- 开发时需要遵循Qt6的API规范，注意与Qt5的差异
- 作为工业级应用，需重点关注错误处理和异常安全性