#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-smoke}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"
BUILD_JOBS="${BUILD_JOBS:-8}"
RUN_APP_SMOKE="${RUN_APP_SMOKE:-0}"
BIN="$BUILD_DIR/hostcomputer"
LOG_DIR="$ROOT/test-logs"
LOG_FILE="$LOG_DIR/smoke_$(date +%Y%m%d_%H%M%S).log"

mkdir -p "$LOG_DIR"

echo "[1/4] 配置 CMake..."
cmake_args=(-S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE")
if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
  cmake_args+=(-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH")
elif [[ -d "$HOME/Qt/6.7.3/gcc_64" ]]; then
  cmake_args+=(-DCMAKE_PREFIX_PATH="$HOME/Qt/6.7.3/gcc_64")
fi
cmake "${cmake_args[@]}" | tee -a "$LOG_FILE"

echo "[2/4] 编译..."
cmake --build "$BUILD_DIR" -j"$BUILD_JOBS" | tee -a "$LOG_FILE"

echo "[3/4] 检查可执行文件..."
if [[ ! -x "$BIN" ]]; then
  echo "❌ 可执行文件不存在: $BIN" | tee -a "$LOG_FILE"
  exit 1
fi

if [[ "$RUN_APP_SMOKE" != "1" ]]; then
  echo "[4/4] ✅ 构建冒烟通过（跳过GUI启动；设置 RUN_APP_SMOKE=1 可启用）" | tee -a "$LOG_FILE"
  echo "日志: $LOG_FILE"
  exit 0
fi

echo "[4/4] 启动GUI冒烟测试(8秒)..."
set +e
python3 - <<PY >> "$LOG_FILE" 2>&1
import subprocess, sys
bin_path = r"$BIN"
try:
    p = subprocess.Popen([bin_path])
    p.wait(timeout=8)
    code = p.returncode
except subprocess.TimeoutExpired:
    p.terminate()
    try:
        p.wait(timeout=2)
    except subprocess.TimeoutExpired:
        p.kill()
    code = 124
sys.exit(code)
PY
RUN_CODE=$?
set -e

if [[ $RUN_CODE -eq 0 || $RUN_CODE -eq 124 || $RUN_CODE -eq 143 ]]; then
  echo "✅ GUI冒烟通过（程序可启动）" | tee -a "$LOG_FILE"
  echo "日志: $LOG_FILE"
  exit 0
else
  echo "❌ GUI冒烟失败，退出码: $RUN_CODE" | tee -a "$LOG_FILE"
  echo "日志: $LOG_FILE"
  exit $RUN_CODE
fi
