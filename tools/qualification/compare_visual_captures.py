#!/usr/bin/env python3
"""Compare two ReFusion PPM qualification captures without third-party code."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]
DEFAULT_POLICY = (
    ROOT / "contracts" / "visual" / "desktop-v1-pixel-tolerance.json"
)


def next_token(data: bytes, offset: int) -> tuple[bytes, int]:
    while offset < len(data):
        if data[offset] == ord("#"):
            newline = data.find(b"\n", offset)
            offset = len(data) if newline < 0 else newline + 1
        elif chr(data[offset]).isspace():
            offset += 1
        else:
            break
    start = offset
    while offset < len(data) and not chr(data[offset]).isspace():
        offset += 1
    if start == offset:
        raise ValueError("PPM header ended before the next token")
    return data[start:offset], offset


def read_ppm(path: pathlib.Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    offset = 0
    magic, offset = next_token(data, offset)
    width_token, offset = next_token(data, offset)
    height_token, offset = next_token(data, offset)
    maximum_token, offset = next_token(data, offset)
    if magic != b"P6" or maximum_token != b"255":
        raise ValueError(f"{path} is not an RGB8 P6 PPM")
    width = int(width_token)
    height = int(height_token)
    if width <= 0 or height <= 0:
        raise ValueError(f"{path} has invalid dimensions")
    if offset >= len(data) or not chr(data[offset]).isspace():
        raise ValueError(f"{path} has no header/pixel delimiter")
    offset += 1
    pixels = data[offset:]
    expected = width * height * 3
    if len(pixels) != expected:
        raise ValueError(
            f"{path} has {len(pixels)} pixel bytes; expected {expected}"
        )
    return width, height, pixels


def luminance(rgb: bytes, pixel: int) -> float:
    offset = pixel * 3
    return (
        0.2126 * rgb[offset]
        + 0.7152 * rgb[offset + 1]
        + 0.0722 * rgb[offset + 2]
    )


def global_ssim(reference: bytes, candidate: bytes) -> float:
    count = len(reference) // 3
    reference_values = [luminance(reference, pixel) for pixel in range(count)]
    candidate_values = [luminance(candidate, pixel) for pixel in range(count)]
    mean_reference = sum(reference_values) / count
    mean_candidate = sum(candidate_values) / count
    denominator = max(1, count - 1)
    variance_reference = sum(
        (value - mean_reference) ** 2 for value in reference_values
    ) / denominator
    variance_candidate = sum(
        (value - mean_candidate) ** 2 for value in candidate_values
    ) / denominator
    covariance = sum(
        (left - mean_reference) * (right - mean_candidate)
        for left, right in zip(reference_values, candidate_values)
    ) / denominator
    c1 = (0.01 * 255.0) ** 2
    c2 = (0.03 * 255.0) ** 2
    return (
        (2.0 * mean_reference * mean_candidate + c1)
        * (2.0 * covariance + c2)
        / (
            (mean_reference**2 + mean_candidate**2 + c1)
            * (variance_reference + variance_candidate + c2)
        )
    )


def compare(
    reference_path: pathlib.Path,
    candidate_path: pathlib.Path,
    policy_path: pathlib.Path,
) -> dict[str, object]:
    reference_width, reference_height, reference = read_ppm(reference_path)
    candidate_width, candidate_height, candidate = read_ppm(candidate_path)
    policy = json.loads(policy_path.read_text(encoding="utf-8"))
    same_dimensions = (
        reference_width == candidate_width
        and reference_height == candidate_height
    )
    if not same_dimensions:
        return {
            "schema": "refusion.xplat-pixel-comparison.v1",
            "policy_id": policy.get("policy_id"),
            "passed": False,
            "code": "RFX-XPLAT-PIXEL-DIMENSIONS-001",
            "reference_dimensions": [reference_width, reference_height],
            "candidate_dimensions": [candidate_width, candidate_height],
        }

    differences = [
        abs(left - right) for left, right in zip(reference, candidate)
    ]
    pixel_count = reference_width * reference_height
    over_three = sum(
        1
        for pixel in range(pixel_count)
        if max(differences[pixel * 3 : pixel * 3 + 3]) > 3
    )
    maximum = max(differences, default=0)
    mean = sum(differences) / max(1, len(differences))
    over_ratio = over_three / max(1, pixel_count)
    ssim = global_ssim(reference, candidate)
    thresholds = policy["metrics"]
    passed = (
        maximum <= thresholds["maximum_channel_delta"]
        and mean <= thresholds["mean_absolute_channel_delta"]
        and over_ratio <= thresholds["pixels_over_delta_3_ratio"]
        and ssim >= thresholds["minimum_ssim"]
    )
    return {
        "schema": "refusion.xplat-pixel-comparison.v1",
        "policy_id": policy["policy_id"],
        "passed": passed,
        "code": (
            "RFX-XPLAT-PIXEL-MATCHED"
            if passed
            else "RFX-XPLAT-PIXEL-TOLERANCE-001"
        ),
        "reference": {
            "path": reference_path.as_posix(),
            "sha256": hashlib.sha256(reference_path.read_bytes()).hexdigest(),
        },
        "candidate": {
            "path": candidate_path.as_posix(),
            "sha256": hashlib.sha256(candidate_path.read_bytes()).hexdigest(),
        },
        "dimensions": [reference_width, reference_height],
        "metrics": {
            "maximum_channel_delta": maximum,
            "mean_absolute_channel_delta": mean,
            "pixels_over_delta_3_ratio": over_ratio,
            "ssim": ssim,
            "exact": reference == candidate,
        },
        "thresholds": thresholds,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("reference", type=pathlib.Path)
    parser.add_argument("candidate", type=pathlib.Path)
    parser.add_argument("--policy", type=pathlib.Path, default=DEFAULT_POLICY)
    parser.add_argument("--output", type=pathlib.Path)
    arguments = parser.parse_args()
    try:
        result = compare(
            arguments.reference, arguments.candidate, arguments.policy
        )
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"capture comparison failed: {error}", file=sys.stderr)
        return 2
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if arguments.output:
        arguments.output.write_text(rendered, encoding="utf-8")
    else:
        sys.stdout.write(rendered)
    return 0 if result.get("passed") else 1


if __name__ == "__main__":
    raise SystemExit(main())
