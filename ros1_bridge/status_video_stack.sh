#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PID_DIR="${PID_DIR:-/tmp/robot_video_stack}"
LOG_DIR="${LOG_DIR:-/tmp/rtsp_logs}"
VIDEO_MANAGER_HOST="${VIDEO_MANAGER_HOST:-127.0.0.1}"
VIDEO_MANAGER_PORT="${VIDEO_MANAGER_PORT:-18081}"
BRIDGE_PORT="${BRIDGE_PORT:-9090}"
DEBUG_PORT="${DEBUG_PORT:-18080}"
RTSP_PORT="${RTSP_PORT:-8554}"

show_pid() {
  local name="$1"
  local pid_file="$PID_DIR/${name}.pid"
  if [[ -s "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null; then
    echo "${name}: running pid=$(cat "$pid_file")"
  else
    echo "${name}: stopped"
  fi
}

show_port() {
  local name="$1"
  local host="$2"
  local port="$3"
  if python3 - "$host" "$port" <<'PY'
import socket
import sys
try:
    with socket.create_connection((sys.argv[1], int(sys.argv[2])), timeout=0.2):
        pass
except OSError:
    sys.exit(1)
PY
  then
    echo "${name}: listening on ${host}:${port}"
  else
    echo "${name}: not listening on ${host}:${port}"
  fi
}

echo "== processes =="
show_pid video_manager_node
show_pid host_bridge_node

echo
echo "== ports =="
show_port MediaMTX 127.0.0.1 "$RTSP_PORT"
show_port video_manager_node "$VIDEO_MANAGER_HOST" "$VIDEO_MANAGER_PORT"
show_port host_bridge_node 127.0.0.1 "$BRIDGE_PORT"
show_port host_bridge_debug 127.0.0.1 "$DEBUG_PORT"

echo
echo "== video diagnostics =="
PYTHONPATH="$SCRIPT_DIR:${PYTHONPATH:-}" python3 - "$VIDEO_MANAGER_HOST" "$VIDEO_MANAGER_PORT" <<'PY'
import sys
from video_manager_ipc import VideoManagerClient, VideoManagerClientError

host = sys.argv[1]
port = int(sys.argv[2])
try:
    diag = VideoManagerClient(host, port, timeout=1.0).diagnostics()
except VideoManagerClientError as exc:
    print(f"video manager unavailable: {exc}")
    sys.exit(0)

summary = diag.get("summary", {})
rtsp = diag.get("rtsp", {})
print(
    "summary: "
    f"online={summary.get('online', 0)}/"
    f"{summary.get('total', 0)} "
    f"desired={summary.get('desired_online', 0)} "
    f"errors={summary.get('errors', 0)} "
    f"mediamtx_listening={diag.get('mediamtx_listening')}"
)
print(
    "rtsp: "
    f"public={rtsp.get('public_host')} "
    f"publish={rtsp.get('publish_host')}:{rtsp.get('port')} "
    f"transport={rtsp.get('transport')}"
)
for camera in diag.get("cameras", []):
    err = camera.get("last_error") or ""
    if len(err) > 180:
        err = err[:177] + "..."
    print(
        "camera "
        f"{camera.get('camera_id')} {camera.get('source_id')} "
        f"online={camera.get('online')} "
        f"desired={camera.get('desired_online')} "
        f"pid={camera.get('pid', 0)} "
        f"log={camera.get('log_path', '')} "
        f"error={err}"
    )
PY

echo
echo "== ffmpeg publishers =="
pgrep -af '^ffmpeg .*rtsp' || true

echo
echo "== recent node logs =="
for log in "$LOG_DIR/video_manager_node.log" "$LOG_DIR/host_bridge_node.log"; do
  if [[ -f "$log" ]]; then
    echo "--- $log ---"
    tail -40 "$log"
  fi
done
