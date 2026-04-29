#!/usr/bin/env bash
set -euo pipefail

PID_DIR="${PID_DIR:-/tmp/robot_video_stack}"

stop_process() {
  local name="$1"
  local pid_file="$PID_DIR/${name}.pid"
  if [[ ! -s "$pid_file" ]]; then
    echo "${name}: no pid file"
    return 0
  fi

  local pid
  pid="$(cat "$pid_file")"
  if ! kill -0 "$pid" 2>/dev/null; then
    echo "${name}: pid ${pid} is not running"
    rm -f "$pid_file"
    return 0
  fi

  echo "stopping ${name}: pid=${pid}"
  kill "$pid" 2>/dev/null || true
  for _ in $(seq 1 30); do
    if ! kill -0 "$pid" 2>/dev/null; then
      rm -f "$pid_file"
      echo "${name}: stopped"
      return 0
    fi
    sleep 0.2
  done

  echo "${name}: forcing stop"
  kill -9 "$pid" 2>/dev/null || true
  rm -f "$pid_file"
}

stop_process host_bridge_node
stop_process video_manager_node

if [[ "${STOP_MEDIAMTX:-0}" == "1" ]] && command -v systemctl >/dev/null 2>&1; then
  systemctl stop mediamtx >/dev/null 2>&1 || true
fi
