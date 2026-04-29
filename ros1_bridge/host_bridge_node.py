#!/usr/bin/env python3
import argparse
import json
import os
import socket
import threading
from typing import Any, Dict, List, Optional, Tuple

from bridge_core import BridgeCore, BridgeRuntime
from bridge_protocol import DEFAULT_WATCHDOG_MS, MAX_FRAME_BYTES, PROTOCOL_VERSION
from bridge_protocol import json_line
from debug_ui import DebugHttpServer
from debug_events import EventSink, RingBufferEventSink
from output_adapters import DryRunOutput, OutputAdapter, RosOutput

DEFAULT_CAMERA_CONFIG = os.path.join(os.path.dirname(__file__), "video_sources.local.yaml")


class HostBridgeServer:
    def __init__(
        self,
        host: str,
        port: int,
        output: OutputAdapter,
        watchdog_ms: int = DEFAULT_WATCHDOG_MS,
        linear_speed: float = 0.8,
        angular_speed: float = 1.5,
        servo_frame: str = "catch_camera",
        gripper_min_position: float = 0.0,
        gripper_max_position: float = 0.044,
        gripper_initial_position: float = 0.022,
        cameras: Optional[List[Dict[str, Any]]] = None,
        events: Optional[EventSink] = None,
    ) -> None:
        self.host = host
        self.port = port
        self.stop_event = threading.Event()
        self.client_lock = threading.Lock()
        self.active_clients: List["BridgeClient"] = []
        self.events = events or RingBufferEventSink()
        self.core = BridgeCore(
            output,
            watchdog_ms,
            linear_speed,
            angular_speed,
            servo_frame,
            gripper_min_position,
            gripper_max_position,
            gripper_initial_position,
            cameras,
            self.events,
        )
        self.runtime = BridgeRuntime(self.core, self.broadcast)

    def serve_forever(self) -> None:
        self.runtime.start_watchdog()
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind((self.host, self.port))
            server.listen(5)
            print(f"[bridge] listening on {self.host}:{self.port}", flush=True)
            self.events.emit("tcp", "bridge listening", data={"host": self.host, "port": self.port})
            while not self.stop_event.is_set():
                try:
                    conn, addr = server.accept()
                except OSError:
                    break
                client = BridgeClient(self, conn, addr)
                with self.client_lock:
                    self.active_clients.append(client)
                threading.Thread(target=client.run, daemon=True).start()

    def remove_client(self, client: "BridgeClient") -> None:
        with self.client_lock:
            if client in self.active_clients:
                self.active_clients.remove(client)
        self.core.publish_zero("client_disconnected")

    def broadcast(self, payload: Dict[str, Any]) -> None:
        with self.client_lock:
            clients = list(self.active_clients)
        for client in clients:
            client.send_json(payload)


class BridgeClient:
    def __init__(self, server: HostBridgeServer, conn: socket.socket, addr: Tuple[str, int]) -> None:
        self.server = server
        self.conn = conn
        self.addr = addr
        self.send_lock = threading.Lock()
        self.closed = False

    def run(self) -> None:
        print(f"[bridge] client connected: {self.addr}", flush=True)
        self.server.events.emit("tcp", "client connected", data={"addr": f"{self.addr[0]}:{self.addr[1]}"})
        self.conn.settimeout(1.0)
        try:
            self.send_json(self.server.core.make_hello())
            self.send_json(self.server.core.make_capabilities())
            self._recv_loop()
        finally:
            self.closed = True
            try:
                self.conn.close()
            except OSError:
                pass
            self.server.remove_client(self)
            self.server.events.emit("tcp", "client disconnected", data={"addr": f"{self.addr[0]}:{self.addr[1]}"})
            print(f"[bridge] client disconnected: {self.addr}", flush=True)

    def send_json(self, payload: Dict[str, Any]) -> None:
        if self.closed:
            return
        try:
            with self.send_lock:
                self.conn.sendall(json_line(payload))
        except OSError:
            self.closed = True

    def _recv_loop(self) -> None:
        buffer = b""
        while not self.server.stop_event.is_set() and not self.closed:
            try:
                chunk = self.conn.recv(4096)
            except socket.timeout:
                continue
            except OSError:
                break

            if not chunk:
                break

            buffer += chunk
            if len(buffer) > MAX_FRAME_BYTES and b"\n" not in buffer:
                self.server.events.emit("protocol", "frame exceeds max length", level="error")
                self.send_json(self.server.core.make_protocol_error({}, 2001, "TCP frame exceeds max length"))
                break

            while b"\n" in buffer:
                line, buffer = buffer.split(b"\n", 1)
                if not line.strip():
                    continue
                if len(line) > MAX_FRAME_BYTES:
                    self.server.events.emit("protocol", "line exceeds max length", level="error")
                    self.send_json(self.server.core.make_protocol_error({}, 2001, "TCP frame exceeds max length"))
                    return
                try:
                    msg = json.loads(line.decode("utf-8"))
                    if not isinstance(msg, dict):
                        raise ValueError("JSON frame is not an object")
                except Exception as exc:
                    self.server.events.emit("protocol", "invalid JSON", level="error", data={"error": str(exc)})
                    self.send_json(self.server.core.make_protocol_error({}, 2100, f"invalid JSON: {exc}"))
                    continue

                for response in self.server.core.handle_message(msg):
                    self.send_json(response)


def parse_camera(value: str) -> Dict[str, Any]:
    parts = value.split(",", 2)
    if len(parts) != 3:
        raise argparse.ArgumentTypeError("camera format: id,name,rtsp_url")
    camera_id = int(parts[0])
    name = parts[1]
    rtsp_url = parts[2]
    return {
        "camera_id": camera_id,
        "name": name,
        "online": bool(rtsp_url),
        "rtsp_url": rtsp_url,
        "codec": "h264",
        "width": 1280,
        "height": 720,
        "fps": 25,
        "bitrate_kbps": 2500 if rtsp_url else 0,
    }


def load_cameras_from_yaml(path: str, publish_host_override: str = "") -> List[Dict[str, Any]]:
    try:
        import yaml
    except ImportError as exc:
        raise RuntimeError("PyYAML is required for --camera-config") from exc

    with open(path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}

    rtsp = data.get("rtsp", {}) if isinstance(data.get("rtsp", {}), dict) else {}
    publish_host = publish_host_override or rtsp.get("publish_host") or rtsp.get("host") or "127.0.0.1"
    port = int(rtsp.get("port", 8554))

    sources = data.get("direct_sources", [])
    if not isinstance(sources, list):
        raise ValueError("camera config field direct_sources must be a list")

    cameras: List[Dict[str, Any]] = []
    for source in sources:
        if not isinstance(source, dict):
            continue
        if not bool(source.get("enabled", True)):
            continue

        rtsp_path = str(source.get("rtsp_path") or source.get("source_id") or "").strip("/")
        if not rtsp_path:
            raise ValueError("enabled camera source is missing rtsp_path/source_id")

        camera_id = int(source.get("camera_id", len(cameras)))
        name = str(source.get("name") or source.get("source_id") or rtsp_path)
        codec = str(source.get("codec", "h264"))
        width = int(source.get("width", 1280))
        height = int(source.get("height", 720))
        fps = int(source.get("fps", 25))
        bitrate_kbps = int(source.get("bitrate_kbps", 0))

        cameras.append({
            "camera_id": camera_id,
            "name": name,
            "online": True,
            "rtsp_url": f"rtsp://{publish_host}:{port}/{rtsp_path}",
            "codec": codec,
            "width": width,
            "height": height,
            "fps": fps,
            "bitrate_kbps": bitrate_kbps,
        })

    return cameras


def detect_publish_host() -> str:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.connect(("8.8.8.8", 80))
            return sock.getsockname()[0]
    except OSError:
        return "127.0.0.1"


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="ROS1 host bridge TCP/JSON node")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=9090)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true", help="run without ROS and print bridge outputs")
    mode.add_argument("--ros", action="store_true", help="publish ROS control topics with rospy; default mode")
    parser.add_argument("--cmd-vel-topic", default="/cmd_vel")
    parser.add_argument("--servo-topic", default="/servo_server/delta_twist_cmds")
    parser.add_argument("--servo-frame", default="catch_camera")
    parser.add_argument("--gripper-position-topic", default="/arm_control/gripper_position")
    parser.add_argument("--gripper-min-position", type=float, default=0.0)
    parser.add_argument("--gripper-max-position", type=float, default=0.044)
    parser.add_argument("--gripper-initial-position", type=float, default=0.022)
    parser.add_argument("--watchdog-ms", type=int, default=DEFAULT_WATCHDOG_MS)
    parser.add_argument("--linear-speed", type=float, default=0.8, help="vehicle level-5 linear speed")
    parser.add_argument("--angular-speed", type=float, default=1.5, help="vehicle level-5 angular speed")
    parser.add_argument("--debug-ui", dest="debug_ui", action="store_true", default=True,
                        help="start read-only debug HTTP UI; enabled by default")
    parser.add_argument("--no-debug-ui", dest="debug_ui", action="store_false",
                        help="disable debug HTTP UI")
    parser.add_argument("--debug-host", default="0.0.0.0")
    parser.add_argument("--debug-port", type=int, default=18080)
    parser.add_argument(
        "--camera-config",
        default=DEFAULT_CAMERA_CONFIG,
        help="load camera list from a video_sources YAML file",
    )
    parser.add_argument(
        "--camera-publish-host",
        default="",
        help="override rtsp.publish_host when loading --camera-config",
    )
    parser.add_argument(
        "--camera",
        action="append",
        type=parse_camera,
        help="camera entry as id,name,rtsp_url; may be repeated",
    )
    return parser


def main() -> None:
    args = build_arg_parser().parse_args()
    events = RingBufferEventSink()
    cameras = args.camera
    if args.camera_config:
        camera_config = os.path.expanduser(args.camera_config)
        camera_publish_host = args.camera_publish_host or detect_publish_host()
        cameras = load_cameras_from_yaml(camera_config, camera_publish_host)
        if args.camera:
            cameras.extend(args.camera)

    if not args.dry_run:
        output: OutputAdapter = RosOutput(
            "host_bridge_node",
            args.cmd_vel_topic,
            args.servo_topic,
            args.gripper_position_topic,
            events,
        )
    else:
        output = DryRunOutput(events)

    server = HostBridgeServer(
        host=args.host,
        port=args.port,
        output=output,
        watchdog_ms=args.watchdog_ms,
        linear_speed=args.linear_speed,
        angular_speed=args.angular_speed,
        servo_frame=args.servo_frame,
        gripper_min_position=args.gripper_min_position,
        gripper_max_position=args.gripper_max_position,
        gripper_initial_position=args.gripper_initial_position,
        cameras=cameras,
        events=events,
    )
    if args.debug_ui:
        DebugHttpServer(args.debug_host, args.debug_port, server.core, events).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("[bridge] stopping", flush=True)
        server.stop_event.set()
        server.runtime.stop_event.set()


if __name__ == "__main__":
    main()
