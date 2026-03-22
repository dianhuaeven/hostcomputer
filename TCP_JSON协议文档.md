# TCP JSON 协议文档

通信端口：**9090**（默认）
传输方式：TCP 长连接，每条 JSON 消息以 `\n` 结尾
数据编码：UTF-8

---

## 一、上位机 → 下位机（发送）

### 1. motor_command — 电机控制命令

发送6关节+执行器的控制数据。

```json
{
  "type": "motor_command",
  "joints": [
    { "position": 1.0, "current": 0.5 },
    { "position": 2.0, "current": 0.6 },
    { "position": 1.5, "current": 0.4 },
    { "position": 0.8, "current": 0.3 },
    { "position": 1.2, "current": 0.7 },
    { "position": 0.9, "current": 0.5 }
  ],
  "executor_position": 0.5,
  "executor_torque": 1.0,
  "executor_flags": 1,
  "reserved": 0
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| joints | array[6] | 6个关节数据 |
| joints[].position | float | 关节位置（实际值，非×1000） |
| joints[].current | float | 关节电流（实际值） |
| executor_position | float | 执行器位置 |
| executor_torque | float | 执行器扭矩 |
| executor_flags | int | 执行器标志位 |
| reserved | int | 保留字段 |

---

### 2. joint_control — 单关节控制命令

控制单个关节的位置和速度。

```json
{
  "type": "joint_control",
  "joint_id": 0,
  "position": 1.5,
  "velocity": 0.8,
  "timestamp": 1709971200000
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| joint_id | int | 关节ID（0-5） |
| position | float | 目标位置 |
| velocity | float | 目标速度 |
| timestamp | int64 | 毫秒级时间戳 |

---

### 3. cmd_vel — 速度控制命令

控制机器人整体运动速度（键盘控制时发送）。

```json
{
  "type": "cmd_vel",
  "linear_x": 0.5,
  "linear_y": 0.0,
  "angular_z": 0.3,
  "timestamp": 1709971200000
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| linear_x | float | 线速度X（前进/后退） |
| linear_y | float | 线速度Y（左移/右移） |
| angular_z | float | 角速度Z（左转/右转） |
| timestamp | int64 | 毫秒级时间戳 |

---

### 4. emergency_stop — 急停命令

紧急停止所有运动。

```json
{
  "type": "emergency_stop",
  "timestamp": 1709971200000
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| timestamp | int64 | 毫秒级时间戳 |

---

### 5. system_command — 系统命令

通用系统命令，可携带任意参数。

```json
{
  "type": "system_command",
  "command": "reset",
  "params": {
    "mode": "soft"
  },
  "timestamp": 1709971200000
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| command | string | 命令名称 |
| params | object | 命令参数（任意键值对） |
| timestamp | int64 | 毫秒级时间戳 |

---

### 6. heartbeat — 心跳包

每秒自动发送一次，用于保活检测。

```json
{
  "type": "heartbeat",
  "timestamp": 1709971200000
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| timestamp | int64 | 毫秒级时间戳 |

---

## 二、下位机 → 上位机（接收）

### 1. motor_state — 电机状态数据

下位机上报6关节+执行器的实时状态。

```json
{
  "type": "motor_state",
  "joints": [
    { "position": 1.0, "current": 0.5 },
    { "position": 2.0, "current": 0.6 },
    { "position": 1.5, "current": 0.4 },
    { "position": 0.8, "current": 0.3 },
    { "position": 1.2, "current": 0.7 },
    { "position": 0.9, "current": 0.5 }
  ],
  "executor_position": 0.5,
  "executor_torque": 1.0,
  "executor_flags": 1,
  "reserved": 0
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| joints | array[6] | 6个关节状态 |
| joints[].position | float | 关节位置（上位机内部×1000转int16存储） |
| joints[].current | float | 关节电流（上位机内部×1000转int16存储） |
| executor_position | float | 执行器位置 |
| executor_torque | float | 执行器扭矩 |
| executor_flags | int | 执行器标志位 |
| reserved | int | 保留字段 |

---

### 2. joint_data — 单关节数据

下位机上报单个关节的详细数据。

```json
{
  "type": "joint_data",
  "joint_id": 0,
  "position": 1.5,
  "current": 0.5,
  "torque": 0.8
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| joint_id | int | 关节ID（0-5） |
| position | float | 关节位置 |
| current | float | 关节电流 |
| torque | float | 关节扭矩 |

---

### 3. environment / co2 — 环境传感器数据

下位机上报CO2浓度数据。支持两种 `type` 值：`"environment"` 或 `"co2"`。

**格式一（嵌套 data）：**
```json
{
  "type": "environment",
  "data": {
    "co2_ppm": 420.5,
    "timestamp": 1709971200
  }
}
```

**格式二（扁平）：**
```json
{
  "type": "co2",
  "co2_ppm": 420.5,
  "timestamp": 1709971200
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| co2_ppm | float | CO2浓度（ppm） |
| timestamp | int | 秒级时间戳 |

**UI显示阈值：**
| 浓度范围 | 颜色 | 状态 |
|----------|------|------|
| < 800 ppm | 绿色 | 空气良好 |
| 800 - 1500 ppm | 橙色 | 注意通风 |
| > 1500 ppm | 红色 | 浓度过高 |

---

### 4. system_status — 系统状态

下位机上报系统运行状态，内容为任意键值对。

```json
{
  "type": "system_status",
  "cpu_usage": 45,
  "memory_usage": 60,
  "uptime": 3600,
  "ros_status": "running"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| (任意) | any | 由下位机自行定���的状态字段 |

---

### 5. camera_info — 摄像头信息推送

下位机上报摄像头的 RTSP 地址及参数信息，上位机收到后自动在对应格子启动 RTSP 播放。

```json
{
  "type": "camera_info",
  "camera_id": 0,
  "online": true,
  "codec": "h265",
  "width": 1280,
  "height": 720,
  "fps": 30,
  "bitrate_kbps": 2000,
  "rtsp_url": "rtsp://192.168.1.100:8554/cam0"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| camera_id | int | 摄像头ID（0-4），对应上位机 2x3 网格的前5个格子 |
| online | bool | true=上线自动播放，false=离线停止播放 |
| codec | string | 编码格式（如 "h264"、"h265"） |
| width | int | 视频宽度（像素） |
| height | int | 视频高度（像素） |
| fps | int | 帧率 |
| bitrate_kbps | int | 码率（kbps） |
| rtsp_url | string | RTSP 流地址 |

---

## 三、协议约定

1. **传输格式**：每条消息为一行紧凑 JSON，以 `\n` 分隔
2. **必要字段**：所有消息必须包含 `"type"` 字段
3. **时间戳**：发送方向的 `timestamp` 为毫秒级（`QDateTime::currentMSecsSinceEpoch()`），接收方向由下位机决定
4. **心跳机制**：上位机每 1 秒发送一次 heartbeat，若检测到连接断开则自动重连（最多 5 次，间隔 3 秒）
5. **数值精度**：motor_state 中的 position/current 为浮点值，上位机内部乘以 1000 转为 int16 存储
