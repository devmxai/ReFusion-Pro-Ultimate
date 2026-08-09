#!/usr/bin/env python3
"""Generate the repository-owned Desktop video-import v1 corpus.

FFmpeg/ffprobe are offline fixture-generation tools only. They are not ReFusion
build or runtime dependencies. The committed payloads and their SHA-256 values
are the immutable test inputs; regeneration is an explicit corpus update.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import shlex
import shutil
import subprocess
import tempfile
from typing import Any


GENERATOR_VERSION = "refusion-video-import-v1-generator/2"
EXPECTED_FFMPEG_VERSION = "8.1.2"
FIXTURES = (
    ("mp4-vfr-bframes-aac-offset", "source.mp4", "admitted"),
    ("mov-portrait-rotation-aac", "source.mov", "admitted"),
    ("mp4-landscape-1080p60", "source.mp4", "admitted"),
    ("mp4-unsupported-hevc", "source.mp4", "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED"),
    ("mp4-corrupt-truncated", "source.mp4", "RFX-MEDIA-IMPORT-CORRUPT"),
    ("mp4-encrypted-cenc", "source.mp4", "RFX-MEDIA-IMPORT-ENCRYPTED"),
)


def run(command: list[str], *, capture: bool = False) -> str:
    result = subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )
    return result.stdout if capture else ""


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def ffprobe_json(ffprobe: str, path: pathlib.Path, *, packets: bool) -> dict[str, Any]:
    command = [ffprobe, "-v", "error", "-show_streams", "-show_format"]
    if packets:
        command.append("-show_packets")
    command.extend(["-of", "json", str(path)])
    return json.loads(run(command, capture=True))


def normalized_oracle(ffprobe: str, path: pathlib.Path) -> dict[str, Any]:
    raw = ffprobe_json(ffprobe, path, packets=True)
    stream_keys = (
        "index",
        "codec_name",
        "profile",
        "codec_tag_string",
        "width",
        "height",
        "pix_fmt",
        "level",
        "color_range",
        "color_space",
        "color_transfer",
        "color_primaries",
        "sample_rate",
        "channels",
        "r_frame_rate",
        "avg_frame_rate",
        "time_base",
        "start_pts",
        "start_time",
        "duration_ts",
        "duration",
        "nb_frames",
    )
    packet_keys = ("stream_index", "pts", "dts", "duration", "size", "pos", "flags")
    streams = []
    for item in raw.get("streams", []):
        stream = {key: item[key] for key in stream_keys if key in item}
        rotations = [
            side["rotation"]
            for side in item.get("side_data_list", [])
            if "rotation" in side
        ]
        if rotations:
            stream["rotation_degrees"] = rotations[0]
        streams.append(stream)
    packets = [
        {key: item[key] for key in packet_keys if key in item}
        for item in raw.get("packets", [])
    ]
    format_keys = ("format_name", "start_time", "duration", "size")
    format_info = {
        key: raw.get("format", {})[key]
        for key in format_keys
        if key in raw.get("format", {})
    }
    return {
        "schema_version": 1,
        "kind": "provider-assisted-media-index-oracle",
        "payload_sha256": sha256(path),
        "format": format_info,
        "streams": streams,
        "packets_decode_order": packets,
    }


def ffmpeg_command(ffmpeg: str, *arguments: str) -> list[str]:
    return [ffmpeg, "-hide_banner", "-loglevel", "error", "-y", *arguments]


def receipt_command(command: list[str], temporary: pathlib.Path) -> str:
    prefix = str(temporary)
    normalized = [item.replace(prefix, "$TMP") for item in command]
    if normalized and normalized[0].endswith("ffmpeg"):
        normalized[0] = "$FFMPEG"
    return shlex.join(normalized)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ffmpeg", required=True)
    parser.add_argument("--ffprobe", required=True)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    version_line = run([args.ffmpeg, "-version"], capture=True).splitlines()[0]
    if not version_line.startswith(f"ffmpeg version {EXPECTED_FFMPEG_VERSION} "):
        raise SystemExit(
            f"expected FFmpeg {EXPECTED_FFMPEG_VERSION}, received: {version_line}"
        )

    commands: dict[str, list[list[str]]] = {fixture[0]: [] for fixture in FIXTURES}
    with tempfile.TemporaryDirectory(prefix="refusion-video-import-v1-") as temporary:
        temp = pathlib.Path(temporary)

        vfr = temp / "mp4-vfr-bframes-aac-offset.mp4"
        command = ffmpeg_command(
            args.ffmpeg,
            "-f", "lavfi", "-i", "testsrc2=size=640x360:rate=30:duration=3",
            "-f", "lavfi", "-i", "sine=frequency=880:sample_rate=48000:duration=3",
            "-filter:v", "select='not(eq(mod(n,10),0))',setpts=PTS+0.500/TB,setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709:range=tv",
            "-filter:a", "asetpts=PTS+0.625/TB",
            "-map", "0:v:0", "-map", "1:a:0", "-fps_mode", "vfr", "-copyts",
            "-c:v", "libx264", "-threads:v", "1", "-pix_fmt", "yuv420p", "-profile:v", "high",
            "-level:v", "4.0", "-g", "60", "-bf", "3", "-b:v", "1400k",
            "-maxrate", "1800k", "-bufsize", "3600k", "-color_primaries", "bt709",
            "-color_trc", "bt709", "-colorspace", "bt709", "-color_range", "tv",
            "-c:a", "aac", "-threads:a", "1", "-profile:a", "aac_low", "-b:a", "128k",
            "-movflags", "+faststart+write_colr", "-video_track_timescale", "30000",
            "-avoid_negative_ts", "disabled", str(vfr),
        )
        run(command)
        commands["mp4-vfr-bframes-aac-offset"].append(command)

        rotation_source = temp / "rotation-source.mp4"
        command = ffmpeg_command(
            args.ffmpeg,
            "-f", "lavfi", "-i", "testsrc2=size=640x360:rate=30:duration=2",
            "-f", "lavfi", "-i", "sine=frequency=660:sample_rate=44100:duration=2",
            "-map", "0:v:0", "-map", "1:a:0",
            "-filter:v", "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709:range=tv",
            "-c:v", "libx264", "-threads:v", "1",
            "-pix_fmt", "yuv420p", "-profile:v", "high", "-level:v", "4.0",
            "-g", "30", "-bf", "2", "-crf", "21", "-color_primaries", "bt709",
            "-color_trc", "bt709", "-colorspace", "bt709", "-color_range", "tv",
            "-c:a", "aac", "-threads:a", "1", "-profile:a", "aac_low", "-b:a", "96k", "-shortest",
            "-movflags", "+write_colr",
            str(rotation_source),
        )
        run(command)
        rotated = temp / "mov-portrait-rotation-aac.mov"
        commands["mov-portrait-rotation-aac"].append(command)
        command = ffmpeg_command(
            args.ffmpeg,
            "-display_rotation:v:0", "90", "-i", str(rotation_source),
            "-map", "0", "-c", "copy", "-movflags", "+write_colr", str(rotated),
        )
        run(command)
        commands["mov-portrait-rotation-aac"].append(command)

        performance = temp / "mp4-landscape-1080p60.mp4"
        command = ffmpeg_command(
            args.ffmpeg,
            "-f", "lavfi", "-i", "testsrc2=size=1920x1080:rate=60:duration=2",
            "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=48000:duration=2",
            "-map", "0:v:0", "-map", "1:a:0",
            "-filter:v", "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709:range=tv",
            "-c:v", "libx264", "-threads:v", "1",
            "-preset", "veryfast", "-pix_fmt", "yuv420p", "-profile:v", "high",
            "-level:v", "4.2", "-g", "120", "-bf", "3", "-b:v", "8M",
            "-maxrate", "10M", "-bufsize", "20M", "-color_primaries", "bt709",
            "-color_trc", "bt709", "-colorspace", "bt709", "-color_range", "tv",
            "-c:a", "aac", "-threads:a", "1", "-profile:a", "aac_low", "-b:a", "128k", "-shortest",
            "-movflags", "+faststart+write_colr", str(performance),
        )
        run(command)
        commands["mp4-landscape-1080p60"].append(command)

        hevc = temp / "mp4-unsupported-hevc.mp4"
        command = ffmpeg_command(
            args.ffmpeg,
            "-f", "lavfi", "-i", "testsrc2=size=320x180:rate=30:duration=1",
            "-an", "-c:v", "libx265", "-threads:v", "1", "-preset", "ultrafast", "-pix_fmt", "yuv420p",
            "-tag:v", "hvc1", "-color_primaries", "bt709", "-color_trc", "bt709",
            "-colorspace", "bt709", "-color_range", "tv", str(hevc),
        )
        run(command)
        commands["mp4-unsupported-hevc"].append(command)

        encrypted = temp / "mp4-encrypted-cenc.mp4"
        command = ffmpeg_command(
            args.ffmpeg,
            "-i", str(vfr), "-map", "0", "-c", "copy",
            "-encryption_scheme", "cenc-aes-ctr",
            "-encryption_key", "000102030405060708090a0b0c0d0e0f",
            "-encryption_kid", "00112233445566778899aabbccddeeff",
            str(encrypted),
        )
        run(command)
        commands["mp4-encrypted-cenc"].append(command)

        corrupt = temp / "mp4-corrupt-truncated.mp4"
        original = vfr.read_bytes()
        corrupt.write_bytes(original[:-8192])
        commands["mp4-corrupt-truncated"].append(
            ["python-byte-truncate", "--source", vfr.name, "--remove-tail-bytes", "8192"]
        )

        sources = {
            "mp4-vfr-bframes-aac-offset": vfr,
            "mov-portrait-rotation-aac": rotated,
            "mp4-landscape-1080p60": performance,
            "mp4-unsupported-hevc": hevc,
            "mp4-corrupt-truncated": corrupt,
            "mp4-encrypted-cenc": encrypted,
        }

        if not all(marker in encrypted.read_bytes() for marker in (b"sinf", b"schm", b"tenc", b"senc")):
            raise SystemExit("encrypted fixture is missing required CENC box markers")

        for fixture_id, payload_name, expected in FIXTURES:
            destination = args.output / fixture_id
            destination.mkdir(parents=True, exist_ok=True)
            payload = destination / payload_name
            shutil.copyfile(sources[fixture_id], payload)

            if expected == "admitted":
                oracle = normalized_oracle(args.ffprobe, payload)
            elif fixture_id == "mp4-unsupported-hevc":
                probe = ffprobe_json(args.ffprobe, payload, packets=False)
                oracle = {
                    "schema_version": 1,
                    "expected_diagnostic": expected,
                    "observed_codec": probe["streams"][0]["codec_name"],
                    "observed_codec_tag": probe["streams"][0]["codec_tag_string"],
                }
            elif fixture_id == "mp4-corrupt-truncated":
                oracle = {
                    "schema_version": 1,
                    "expected_diagnostic": expected,
                    "construction": "accepted-source-tail-truncated",
                    "removed_tail_bytes": 8192,
                    "original_source_sha256": sha256(vfr),
                }
            else:
                markers = ["sinf", "schm", "tenc", "senc", "saiz", "saio"]
                data = payload.read_bytes()
                oracle = {
                    "schema_version": 1,
                    "expected_diagnostic": expected,
                    "required_iso_bmff_box_markers": markers,
                    "all_markers_present": all(marker.encode("ascii") in data for marker in markers),
                }

            oracle_path = destination / "expected-index-or-diagnostic.json"
            oracle_path.write_text(json.dumps(oracle, indent=2, sort_keys=True) + "\n")
            fixture = {
                "schema_version": 1,
                "id": fixture_id,
                "payload": payload_name,
                "sha256": sha256(payload),
                "byte_size": payload.stat().st_size,
                "expected": expected,
                "oracle": oracle_path.name,
                "oracle_sha256": sha256(oracle_path),
                "ownership": "ReFusion-generated synthetic audiovisual test pattern",
                "license": "CC0-1.0",
                "generation": {
                    "generator": GENERATOR_VERSION,
                    "ffmpeg_version": EXPECTED_FFMPEG_VERSION,
                    "purpose": "offline fixture generation only; FFmpeg is not a ReFusion build or runtime dependency",
                    "commands": [receipt_command(item, temp) for item in commands[fixture_id]],
                },
            }
            (destination / "fixture.json").write_text(
                json.dumps(fixture, indent=2, sort_keys=True) + "\n"
            )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
