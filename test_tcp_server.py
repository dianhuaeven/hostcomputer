"""
TCP测试服务端 - 模拟ROS节点发送JSON数据
用法: python test_tcp_server.py [端口号]
默认端口: 9090

上位机通过 菜单→连接→TCP连接(ROS) 连接到本机 127.0.0.1:9090
"""

import socket
import json
import time
import sys
import threading

def get_port():
    if len(sys.argv) > 1:
        return int(sys.argv[1])
    return 9090

def send_json(conn, data):
    """发送JSON数据，以换行符结尾"""
    msg = json.dumps(data, ensure_ascii=False) + "\n"
    conn.sendall(msg.encode("utf-8"))
    print(f"  [发送] {msg.strip()}")

def handle_client(conn, addr):
    """处理客户端连接"""
    print(f"\n[连接] 上位机已连接: {addr}")
    print("=" * 60)

    # 启动接收线程（处理心跳等）
    def recv_loop():
        try:
            buf = b""
            while True:
                data = conn.recv(4096)
                if not data:
                    break
                buf += data
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    if line.strip():
                        msg = json.loads(line)
                        msg_type = msg.get("type", "unknown")
                        if msg_type == "heartbeat":
                            pass  # 心跳包静默处理
                        elif msg_type == "cmd_vel":
                            lx = msg.get("linear_x", 0)
                            az = msg.get("angular_z", 0)
                            bar_lx = ">" * int(abs(lx) * 10) if lx > 0 else "<" * int(abs(lx) * 10)
                            bar_az = ")" * int(abs(az) * 5) if az < 0 else "(" * int(abs(az) * 5)
                            direction = ""
                            if lx > 0: direction += "前进"
                            elif lx < 0: direction += "后退"
                            if az > 0: direction += "左转"
                            elif az < 0: direction += "右转"
                            if not direction: direction = "停止"
                            print(f"  [cmd_vel] {direction:8s} lx={lx:+.2f} az={az:+.2f}  {bar_lx}{bar_az}")
                        elif msg_type == "emergency_stop":
                            print(f"  [!!!急停!!!] {msg}")
                        else:
                            print(f"  [收到] {msg}")
        except (ConnectionResetError, ConnectionAbortedError):
            pass
        except Exception as e:
            print(f"  [接收异常] {e}")

    recv_thread = threading.Thread(target=recv_loop, daemon=True)
    recv_thread.start()

    try:
        # === 测试1: 发送 motor_state ===
        print("\n--- 测试1: 发送 motor_state ---")
        motor_state = {
            "type": "motor_state",
            "joints": [
                {"position": 1.5,   "current": 0.3},
                {"position": 2.0,   "current": 0.5},
                {"position": -1.2,  "current": 0.4},
                {"position": 0.8,   "current": 0.6},
                {"position": 3.14,  "current": 0.2},
                {"position": -0.5,  "current": 0.1},
            ],
            "executor_position": 1.0,
            "executor_torque": 2.5,
            "executor_flags": 1,
            "reserved": 0
        }
        send_json(conn, motor_state)
        time.sleep(1)

        # === 测试2: 发送 joint_data ===
        print("\n--- 测试2: 发送 joint_data ---")
        joint_data = {
            "type": "joint_data",
            "joint_id": 0,
            "position": 1.234,
            "current": 0.567,
            "torque": 0.89
        }
        send_json(conn, joint_data)
        time.sleep(1)

        # === 测试3: 发送 system_status ===
        print("\n--- 测试3: 发送 system_status ---")
        system_status = {
            "type": "system_status",
            "cpu_temp": 55.3,
            "motor_driver": "OK",
            "battery": 75,
            "ros_version": "noetic"
        }
        send_json(conn, system_status)
        time.sleep(1)

        print("\n" + "=" * 60)
        print("[完成] 3条测试JSON已发送完毕")
        print("请查看上位机的 '下位机指令' 和 '系统消息' 窗口确认接收情况")
        print("按回车发送更多数据, 输入 'q' 断开连接...")

        while True:
            cmd = input("\n> ").strip()
            if cmd == "q":
                break
            elif cmd == "1":
                print("发送 motor_state...")
                # 随机变化数据
                import random
                for j in motor_state["joints"]:
                    j["position"] = round(random.uniform(-3.14, 3.14), 3)
                    j["current"] = round(random.uniform(0, 1), 3)
                motor_state["executor_position"] = round(random.uniform(0, 2), 3)
                motor_state["executor_torque"] = round(random.uniform(0, 5), 3)
                send_json(conn, motor_state)
            elif cmd == "2":
                print("发送 system_status...")
                send_json(conn, system_status)
            else:
                # 默认再发一次 motor_state
                send_json(conn, motor_state)

    except (ConnectionResetError, ConnectionAbortedError, BrokenPipeError):
        print("\n[断开] 上位机断开连接")
    except KeyboardInterrupt:
        print("\n[中断] 用户中断")
    finally:
        conn.close()

def main():
    port = get_port()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", port))
    server.listen(1)

    print(f"TCP测试服务端已启动，监听端口: {port}")
    print(f"请在上位机中连接到 127.0.0.1:{port}")
    print("等待上位机连接...\n")

    try:
        while True:
            conn, addr = server.accept()
            handle_client(conn, addr)
            print("\n等待下一次连接...")
    except KeyboardInterrupt:
        print("\n服务端关闭")
    finally:
        server.close()

if __name__ == "__main__":
    main()
