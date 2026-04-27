#!/usr/bin/env python3
import json
import os
import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 9090
DROP_INTERVAL_SEC = 15
ACK_MODE = os.getenv("MOCK_ACK_MODE", "ok").lower()
ACK_DELAY_SEC = float(os.getenv("MOCK_ACK_DELAY_SEC", "0"))
CRITICAL_COMMANDS = {
    "emergency_stop",
    "clear_emergency",
    "motor_init",
    "motor_enable",
    "motor_disable",
    "set_control_mode",
    "home_arm",
    "start_task",
    "stop_task",
    "executor_action",
    "set_param",
}


def now_ms():
    return int(time.time() * 1000)


def send_json(conn, payload):
    conn.sendall((json.dumps(payload, ensure_ascii=False) + "\n").encode("utf-8"))


def make_ack(msg, ok=True, code=0, message="ok"):
    return {
        "type": "ack",
        "protocol_version": 1,
        "ack_type": msg.get("type", "unknown"),
        "seq": msg.get("seq", 0),
        "ok": ok,
        "code": code,
        "message": message,
        "timestamp_ms": now_ms(),
    }


def maybe_send_ack(conn, msg, message="ok"):
    if ACK_MODE == "drop":
        print(f"[mock] drop ack for seq={msg.get('seq', 0)} type={msg.get('type', 'unknown')}")
        return

    if ACK_DELAY_SEC > 0:
        time.sleep(ACK_DELAY_SEC)

    if ACK_MODE == "fail":
        send_json(conn, make_ack(msg, ok=False, code=500, message="mock ack failure"))
        return

    send_json(conn, make_ack(msg, message=message))


def client_worker(conn, addr):
    print(f"[mock] client connected: {addr}")
    conn.settimeout(1.0)
    started = time.time()
    try:
        while True:
            if time.time() - started > DROP_INTERVAL_SEC:
                print("[mock] force drop connection for reconnect test")
                conn.close()
                return

            try:
                data = conn.recv(4096)
            except socket.timeout:
                continue

            if not data:
                return

            for line in data.split(b"\n"):
                if not line.strip():
                    continue
                try:
                    msg = json.loads(line.decode("utf-8", errors="ignore"))
                except Exception:
                    continue

                mtype = msg.get("type", "unknown")
                if mtype == "heartbeat":
                    send_json(conn, {
                        "type": "heartbeat_ack",
                        "protocol_version": 1,
                        "seq": msg.get("seq", 0),
                        "timestamp_ms": now_ms(),
                        "server_time_ms": now_ms(),
                    })
                elif mtype == "operator_input":
                    send_json(conn, {
                        "type": "system_status",
                        "mock": True,
                        "last_operator_input_seq": msg.get("seq", 0),
                        "timestamp_ms": now_ms(),
                    })
                elif mtype in CRITICAL_COMMANDS:
                    maybe_send_ack(conn, msg)
                elif mtype == "system_command":
                    maybe_send_ack(conn, msg, message=f"system command {msg.get('command', '')} accepted")
                else:
                    send_json(conn, {
                        "type": "protocol_error",
                        "protocol_version": 1,
                        "seq": msg.get("seq", 0),
                        "code": 1001,
                        "message": f"unsupported message type: {mtype}",
                        "timestamp_ms": now_ms(),
                    })
    finally:
        try:
            conn.close()
        except Exception:
            pass
        print(f"[mock] client disconnected: {addr}")


def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen(5)
        print(f"[mock] ROS1 mock server listening on {HOST}:{PORT}")
        while True:
            conn, addr = s.accept()
            t = threading.Thread(target=client_worker, args=(conn, addr), daemon=True)
            t.start()


if __name__ == "__main__":
    main()
