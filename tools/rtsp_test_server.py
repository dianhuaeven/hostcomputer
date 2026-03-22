#!/usr/bin/env python3
"""
RTSP 视频推流 + 模拟下位机 TCP 服务器
======================================
功能：
  1. 用 FFmpeg 生成带实时时间戳水印的测试视频，推送到本地 RTSP 服务器
  2. 启动 TCP 服务器（模拟下位机），等待上位机连接后推送 camera_info、心跳等

架构：
  上位机 (Qt App)  ---connectToHost--->  本脚本 (TCP Server :9090)
  本脚本 (FFmpeg)  ---RTSP推流--->  mediamtx (:8554)  <---RTSP拉流--- 上位机

用法：
  python rtsp_test_server.py                    # 默认1路
  python rtsp_test_server.py -n 3               # 3路摄像头
  python rtsp_test_server.py --no-tcp           # 只推流，不启动TCP服务器
"""

import argparse
import json
import socket
import subprocess
import sys
import time
import shutil
import signal
import threading

# RTSP 服务器端口（需要先启动 mediamtx）
RTSP_PORT = 8554
RTSP_SERVER = "127.0.0.1"

# 模拟下位机 TCP 监听端口
TCP_PORT = 9090


def check_ffmpeg():
    """检查 FFmpeg 是否可用"""
    if shutil.which("ffmpeg") is None:
        print("[错误] 未找到 ffmpeg，请先安装并添加到 PATH")
        print("  下载地址: https://www.gyan.dev/ffmpeg/builds/")
        sys.exit(1)
    print("[OK] FFmpeg 已就绪")


def start_ffmpeg_stream(camera_id: int, width=1280, height=720, fps=30, rtsp_port=RTSP_PORT):
    """启动一路 FFmpeg 测试推流，带实时时间戳水印"""
    rtsp_url = f"rtsp://{RTSP_SERVER}:{rtsp_port}/cam{camera_id}"

    drawtext = (
        "drawtext=fontsize=48:fontcolor=white:box=1:boxcolor=black@0.7:"
        "boxborderw=8:x=(w-text_w)/2:y=h-th-40:"
        f"text='CAM{camera_id} %{{localtime\\:%H\\\\\\:%M\\\\\\:%S}}.%{{eif\\:mod(t*1000\\,1000)\\:d\\:3}}'"
    )

    cmd = [
        "ffmpeg",
        "-re",
        "-f", "lavfi",
        "-i", f"testsrc2=size={width}x{height}:rate={fps}",
        "-vf", drawtext,
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-tune", "zerolatency",
        "-g", str(fps),
        "-b:v", "2000k",
        "-f", "rtsp",
        "-rtsp_transport", "tcp",
        rtsp_url,
    ]

    print(f"[推流] cam{camera_id} -> {rtsp_url}")
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    return proc, rtsp_url


def build_camera_info(camera_id, rtsp_url, online=True, codec="h264",
                      width=1280, height=720, fps=30, bitrate=2000):
    """构造 camera_info JSON"""
    return json.dumps({
        "type": "camera_info",
        "camera_id": camera_id,
        "online": online,
        "codec": codec,
        "width": width,
        "height": height,
        "fps": fps,
        "bitrate_kbps": bitrate,
        "rtsp_url": rtsp_url,
    }) + "\n"


def build_heartbeat():
    """构造心跳 JSON"""
    return json.dumps({"type": "heartbeat", "timestamp": int(time.time())}) + "\n"


def handle_client(conn, addr, cameras, args):
    """处理一个上位机连接：发送 camera_info + 持续心跳"""
    print(f"[TCP] 上位机已连接: {addr}")

    try:
        # 先发送所有摄像头信息
        time.sleep(0.5)
        for cam_id, rtsp_url in cameras:
            msg = build_camera_info(cam_id, rtsp_url,
                                    width=args.width, height=args.height,
                                    fps=args.fps)
            conn.sendall(msg.encode("utf-8"))
            print(f"[TCP] 已推送 camera_info cam{cam_id} -> {addr}")
            time.sleep(0.1)

        # 持续发送心跳，保持连接
        while True:
            try:
                conn.sendall(build_heartbeat().encode("utf-8"))
                time.sleep(2)
            except (BrokenPipeError, ConnectionResetError):
                break

    except Exception as e:
        print(f"[TCP] 连接异常: {e}")
    finally:
        print(f"[TCP] 上位机断开: {addr}")
        conn.close()


def start_tcp_server(tcp_port, cameras, args):
    """启动 TCP 服务器，等待上位机连接"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", tcp_port))
    server.listen(5)
    print(f"[TCP] 模拟下位机已启动，监听 0.0.0.0:{tcp_port}，等待上位机连接...")

    while True:
        try:
            conn, addr = server.accept()
            t = threading.Thread(target=handle_client, args=(conn, addr, cameras, args),
                                 daemon=True)
            t.start()
        except OSError:
            break


def main():
    parser = argparse.ArgumentParser(description="RTSP 推流 + 模拟下位机 TCP 服务器")
    parser.add_argument("-n", "--num-cameras", type=int, default=1,
                        help="模拟摄像头数量 (0-4, 默认1)")
    parser.add_argument("--port", type=int, default=TCP_PORT,
                        help="TCP 监听端口 (默认 9090)")
    parser.add_argument("--rtsp-port", type=int, default=RTSP_PORT,
                        help="RTSP 服务器端口 (默认 8554)")
    parser.add_argument("--width", type=int, default=1280, help="视频宽度")
    parser.add_argument("--height", type=int, default=720, help="视频高度")
    parser.add_argument("--fps", type=int, default=30, help="帧率")
    parser.add_argument("--no-tcp", action="store_true",
                        help="只推流，不启动 TCP 服务器")
    args = parser.parse_args()

    tcp_port = args.port
    rtsp_port = args.rtsp_port

    check_ffmpeg()

    num = min(args.num_cameras, 5)
    ffmpeg_procs = []
    cameras = []  # [(cam_id, rtsp_url), ...]

    print(f"\n=== 启动 {num} 路 RTSP 测试推流 ===")
    print(f"  RTSP: rtsp://{RTSP_SERVER}:{rtsp_port}/cam0 ~ cam{num - 1}")
    print(f"  分辨率: {args.width}x{args.height} @ {args.fps}fps")
    if not args.no_tcp:
        print(f"  TCP 服务器: 0.0.0.0:{tcp_port}")
    print(f"  提示: 需要先启动 mediamtx (RTSP 服务器)")
    print()

    # 启动推流
    for i in range(num):
        proc, url = start_ffmpeg_stream(i, args.width, args.height, args.fps, rtsp_port)
        ffmpeg_procs.append(proc)
        cameras.append((i, url))
        time.sleep(0.5)

    # 启动 TCP 服务器线程
    if not args.no_tcp:
        tcp_thread = threading.Thread(target=start_tcp_server,
                                      args=(tcp_port, cameras, args),
                                      daemon=True)
        tcp_thread.start()

    print("\n[运行中] 按 Ctrl+C 停止\n")
    print("  上位机连接步骤:")
    print(f"    1. 打开上位机 -> TCP连接 -> 输入 127.0.0.1:{tcp_port}")
    print("    2. 连接成功后会自动推送摄像头信息并开始播放")
    print()

    def cleanup(sig=None, frame=None):
        print("\n[停止] 正在关闭...")
        for proc in ffmpeg_procs:
            proc.terminate()
        print("[停止] 已退出")
        sys.exit(0)

    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    # 监控 FFmpeg 进程
    while True:
        for idx, proc in enumerate(ffmpeg_procs):
            ret = proc.poll()
            if ret is not None:
                stderr = proc.stderr.read().decode("utf-8", errors="replace")
                last_lines = "\n".join(stderr.strip().split("\n")[-5:])
                print(f"[警告] cam{idx} FFmpeg 退出 (code={ret})")
                if last_lines:
                    print(f"  {last_lines}")
                ffmpeg_procs[idx] = None
        ffmpeg_procs[:] = [p for p in ffmpeg_procs if p is not None]
        if not ffmpeg_procs:
            print("[错误] 所有推流进程已退出")
            break
        time.sleep(3)


if __name__ == "__main__":
    main()
