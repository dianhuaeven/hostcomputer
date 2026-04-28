import os
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional

try:
    import yaml
except ImportError:  # pragma: no cover - exercised only on minimal systems
    yaml = None


class VideoConfigError(ValueError):
    pass


@dataclass
class RtspConfig:
    host: str = "127.0.0.1"
    publish_host: str = "127.0.0.1"
    port: int = 8554
    server: str = "mediamtx"

    def public_url(self, path: str) -> str:
        return f"rtsp://{self.host}:{self.port}/{path}"

    def publish_url(self, path: str) -> str:
        return f"rtsp://{self.publish_host}:{self.port}/{path}"


@dataclass
class DirectVideoSource:
    camera_id: int
    name: str
    device: str
    rtsp_path: str
    source_id: str = ""
    runner: str = "ffmpeg"
    input_format: str = "mjpeg"
    width: int = 1280
    height: int = 720
    fps: int = 30
    codec: str = "h264"
    bitrate_kbps: int = 2500
    enabled: bool = True
    slot_hint: Optional[int] = None
    ffmpeg_path: str = "ffmpeg"
    extra_input_args: List[str] = field(default_factory=list)
    extra_output_args: List[str] = field(default_factory=list)

    def __post_init__(self) -> None:
        if not self.source_id:
            self.source_id = f"camera_{self.camera_id}"
        if self.slot_hint is None:
            self.slot_hint = self.camera_id

    def camera_info(
        self,
        rtsp: RtspConfig,
        online: bool = False,
        last_error: str = "",
    ) -> Dict[str, Any]:
        return {
            "camera_id": self.camera_id,
            "source_id": self.source_id,
            "slot_hint": self.slot_hint,
            "name": self.name,
            "kind": "direct",
            "online": online,
            "rtsp_url": rtsp.public_url(self.rtsp_path) if online else "",
            "codec": self.codec,
            "width": self.width,
            "height": self.height,
            "fps": self.fps,
            "bitrate_kbps": self.bitrate_kbps if online else 0,
            "profile": "low_latency",
            "last_error": last_error,
        }


@dataclass
class VideoConfig:
    rtsp: RtspConfig
    direct_sources: List[DirectVideoSource]

    def camera_infos(self) -> List[Dict[str, Any]]:
        return [source.camera_info(self.rtsp, online=False) for source in self.direct_sources]


def load_video_config(path: str) -> VideoConfig:
    if yaml is None:
        raise VideoConfigError("PyYAML is required to load video source configuration")
    if not path:
        raise VideoConfigError("video config path is empty")
    if not os.path.exists(path):
        raise VideoConfigError(f"video config not found: {path}")

    with open(path, "r", encoding="utf-8") as handle:
        data = yaml.safe_load(handle) or {}
    if not isinstance(data, dict):
        raise VideoConfigError("video config root must be a mapping")
    return parse_video_config(data)


def parse_video_config(data: Dict[str, Any]) -> VideoConfig:
    rtsp_data = data.get("rtsp", {}) or {}
    if not isinstance(rtsp_data, dict):
        raise VideoConfigError("rtsp must be a mapping")

    host = _required_str(rtsp_data, "host", default="127.0.0.1")
    publish_host = _required_str(rtsp_data, "publish_host", default="127.0.0.1")
    rtsp = RtspConfig(
        host=host,
        publish_host=publish_host,
        port=_positive_int(rtsp_data, "port", default=8554),
        server=_required_str(rtsp_data, "server", default="mediamtx"),
    )

    source_items = data.get("direct_sources", []) or []
    if not isinstance(source_items, list):
        raise VideoConfigError("direct_sources must be a list")

    sources = [parse_direct_source(item, index) for index, item in enumerate(source_items)]
    _validate_unique_camera_ids(sources)
    _validate_unique_rtsp_paths(sources)
    return VideoConfig(rtsp=rtsp, direct_sources=sources)


def parse_direct_source(item: Any, index: int) -> DirectVideoSource:
    if not isinstance(item, dict):
        raise VideoConfigError(f"direct_sources[{index}] must be a mapping")

    camera_id = _positive_int(item, "camera_id")
    if camera_id > 4:
        raise VideoConfigError(f"direct_sources[{index}].camera_id must be between 0 and 4")

    source = DirectVideoSource(
        camera_id=camera_id,
        source_id=_required_str(item, "source_id", default=f"camera_{camera_id}"),
        slot_hint=_optional_int(item, "slot_hint"),
        name=_required_str(item, "name", default=f"Camera {camera_id}"),
        runner=_required_str(item, "runner", default="ffmpeg"),
        device=_required_str(item, "device"),
        input_format=_required_str(item, "input_format", default="mjpeg"),
        width=_positive_int(item, "width", default=1280),
        height=_positive_int(item, "height", default=720),
        fps=_positive_int(item, "fps", default=30),
        codec=_required_str(item, "codec", default="h264"),
        bitrate_kbps=_positive_int(item, "bitrate_kbps", default=2500),
        rtsp_path=_required_str(item, "rtsp_path"),
        enabled=_bool_value(item, "enabled", default=True),
        ffmpeg_path=_required_str(item, "ffmpeg_path", default="ffmpeg"),
        extra_input_args=_string_list(item, "extra_input_args"),
        extra_output_args=_string_list(item, "extra_output_args"),
    )

    if source.runner != "ffmpeg":
        raise VideoConfigError(
            f"direct_sources[{index}].runner={source.runner!r} is not supported yet"
        )
    if "/" in source.rtsp_path.strip("/"):
        raise VideoConfigError(f"direct_sources[{index}].rtsp_path must be a single path segment")
    return source


def _validate_unique_camera_ids(sources: List[DirectVideoSource]) -> None:
    seen: Dict[int, str] = {}
    for source in sources:
        if source.camera_id in seen:
            raise VideoConfigError(
                f"duplicate camera_id {source.camera_id}: {seen[source.camera_id]} and {source.name}"
            )
        seen[source.camera_id] = source.name


def _validate_unique_rtsp_paths(sources: List[DirectVideoSource]) -> None:
    seen: Dict[str, str] = {}
    for source in sources:
        if source.rtsp_path in seen:
            raise VideoConfigError(
                f"duplicate rtsp_path {source.rtsp_path}: {seen[source.rtsp_path]} and {source.name}"
            )
        seen[source.rtsp_path] = source.name


def _required_str(data: Dict[str, Any], key: str, default: Optional[str] = None) -> str:
    value = data.get(key, default)
    if value is None:
        raise VideoConfigError(f"{key} is required")
    if not isinstance(value, str) or not value.strip():
        raise VideoConfigError(f"{key} must be a non-empty string")
    return value.strip()


def _positive_int(data: Dict[str, Any], key: str, default: Optional[int] = None) -> int:
    value = data.get(key, default)
    if value is None:
        raise VideoConfigError(f"{key} is required")
    if isinstance(value, bool):
        raise VideoConfigError(f"{key} must be an integer")
    try:
        parsed = int(value)
    except (TypeError, ValueError) as exc:
        raise VideoConfigError(f"{key} must be an integer") from exc
    if parsed < 0:
        raise VideoConfigError(f"{key} must be >= 0")
    return parsed


def _optional_int(data: Dict[str, Any], key: str) -> Optional[int]:
    if key not in data or data[key] is None:
        return None
    return _positive_int(data, key)


def _bool_value(data: Dict[str, Any], key: str, default: bool) -> bool:
    value = data.get(key, default)
    if not isinstance(value, bool):
        raise VideoConfigError(f"{key} must be true or false")
    return value


def _string_list(data: Dict[str, Any], key: str) -> List[str]:
    value = data.get(key, []) or []
    if not isinstance(value, list):
        raise VideoConfigError(f"{key} must be a list of strings")
    result: List[str] = []
    for item in value:
        if not isinstance(item, str) or not item:
            raise VideoConfigError(f"{key} must contain only non-empty strings")
        result.append(item)
    return result
