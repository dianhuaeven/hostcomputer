#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PID_DIR="${PID_DIR:-/tmp/robot_video_stack}"
LOG_DIR="${LOG_DIR:-/tmp/rtsp_logs}"
VIDEO_SOURCES_CONFIG="${VIDEO_SOURCES_CONFIG:-${SCRIPT_DIR}/video_sources.local.yaml}"
VIDEO_MANAGER_HOST="${VIDEO_MANAGER_HOST:-127.0.0.1}"
VIDEO_MANAGER_PORT="${VIDEO_MANAGER_PORT:-18081}"
BRIDGE_HOST="${BRIDGE_HOST:-0.0.0.0}"
BRIDGE_PORT="${BRIDGE_PORT:-9090}"
DEBUG_HOST="${DEBUG_HOST:-0.0.0.0}"
DEBUG_PORT="${DEBUG_PORT:-18080}"
RTSP_PORT="${RTSP_PORT:-8554}"
RTSP_PUBLISH_HOST="${RTSP_PUBLISH_HOST:-127.0.0.1}"
BRIDGE_MODE="${BRIDGE_MODE:---ros}"

mkdir -p "$PID_DIR" "$LOG_DIR"

if [[ -n "${ROS_SETUP:-}" && -f "${ROS_SETUP}" ]]; then
  # shellcheck disable=SC1090
  source "${ROS_SETUP}"
fi

detect_public_host() {
  python3 - <<'PY'
import socket
try:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.connect(("8.8.8.8", 80))
        print(sock.getsockname()[0])
except OSError:
    print("127.0.0.1")
PY
}

RTSP_PUBLIC_HOST="${RTSP_PUBLIC_HOST:-$(detect_public_host)}"

port_listening() {
  local host="$1"
  local port="$2"
  python3 - "$host" "$port" <<'PY'
import socket
import sys
host = sys.argv[1]
port = int(sys.argv[2])
try:
    with socket.create_connection((host, port), timeout=0.2):
        pass
except OSError:
    sys.exit(1)
PY
}

wait_for_port() {
  local name="$1"
  local host="$2"
  local port="$3"
  local timeout="${4:-8}"
  local deadline=$((SECONDS + timeout))
  until port_listening "$host" "$port"; do
    if (( SECONDS >= deadline )); then
      echo "ERROR: ${name} did not open ${host}:${port}" >&2
      return 1
    fi
    sleep 0.2
  done
}

pid_alive() {
  local pid_file="$1"
  [[ -s "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null
}

start_process() {
  local name="$1"
  local pid_file="$PID_DIR/${name}.pid"
  local log_file="$LOG_DIR/${name}.log"
  shift
  if pid_alive "$pid_file"; then
    echo "${name} already running pid=$(cat "$pid_file")"
    return 0
  fi
  rm -f "$pid_file"
  nohup "$@" >>"$log_file" 2>&1 &
  local pid=$!
  echo "$pid" >"$pid_file"
  echo "started ${name}: pid=${pid} log=${log_file}"
}

ensure_mediamtx() {
  if [[ "${SKIP_MEDIAMTX_START:-0}" != "1" ]] && command -v systemctl >/dev/null 2>&1; then
    if systemctl list-unit-files mediamtx.service >/dev/null 2>&1; then
      systemctl start mediamtx >/dev/null 2>&1 || true
    fi
  fi
  wait_for_port "MediaMTX" "127.0.0.1" "$RTSP_PORT" "${MEDIAMTX_WAIT_SEC:-8}" || {
    echo "MediaMTX is required before starting video streams." >&2
    echo "Recent MediaMTX logs:" >&2
    journalctl -u mediamtx --since '5 minutes ago' --no-pager 2>/dev/null | tail -80 >&2 || true
    exit 1
  }
}

ensure_mediamtx

video_manager_cmd=(
  python3 "$SCRIPT_DIR/video_manager_node.py"
  --host "$VIDEO_MANAGER_HOST"
  --port "$VIDEO_MANAGER_PORT"
  --config "$VIDEO_SOURCES_CONFIG"
  --rtsp-public-host "$RTSP_PUBLIC_HOST"
  --rtsp-publish-host "$RTSP_PUBLISH_HOST"
  --rtsp-port "$RTSP_PORT"
  --log-dir "$LOG_DIR"
  --autostart
)
if [[ "${VIDEO_MANAGER_DRY_RUN:-0}" == "1" ]]; then
  video_manager_cmd+=(--dry-run)
fi

start_process video_manager_node "${video_manager_cmd[@]}"

wait_for_port "video_manager_node" "$VIDEO_MANAGER_HOST" "$VIDEO_MANAGER_PORT" "${VIDEO_MANAGER_WAIT_SEC:-8}"

start_process host_bridge_node \
  python3 "$SCRIPT_DIR/host_bridge_node.py" \
    "$BRIDGE_MODE" \
    --host "$BRIDGE_HOST" \
    --port "$BRIDGE_PORT" \
    --camera-config "" \
    --video-manager \
    --video-manager-host "$VIDEO_MANAGER_HOST" \
    --video-manager-port "$VIDEO_MANAGER_PORT" \
    --debug-ui \
    --debug-host "$DEBUG_HOST" \
    --debug-port "$DEBUG_PORT"

wait_for_port "host_bridge_node" "127.0.0.1" "$BRIDGE_PORT" "${BRIDGE_WAIT_SEC:-8}"

"$SCRIPT_DIR/status_video_stack.sh"
