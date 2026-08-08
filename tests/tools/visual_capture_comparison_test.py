#!/usr/bin/env python3
"""Unit checks for the dependency-free cross-platform capture comparator."""

from __future__ import annotations

import importlib.util
import pathlib
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "refusion_capture_compare",
    ROOT / "tools" / "qualification" / "compare_visual_captures.py",
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("unable to load capture comparison tool")
COMPARE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(COMPARE)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def ppm(path: pathlib.Path, pixels: bytes) -> None:
    path.write_bytes(b"P6\n2 1\n255\n" + pixels)


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="refusion-capture-compare-") as raw:
        root = pathlib.Path(raw)
        reference = root / "reference.ppm"
        exact = root / "exact.ppm"
        changed = root / "changed.ppm"
        # The first channel is ASCII newline; the parser must treat it as a
        # pixel after consuming exactly one PPM header delimiter.
        reference_pixels = bytes((10, 16, 32, 255, 200, 120))
        ppm(reference, reference_pixels)
        ppm(exact, reference_pixels)
        ppm(changed, bytes((255, 255, 255, 0, 0, 0)))
        exact_result = COMPARE.compare(
            reference, exact, COMPARE.DEFAULT_POLICY
        )
        require(exact_result["passed"], "exact captures did not pass")
        require(exact_result["metrics"]["exact"],
                "exact capture identity was not reported")
        changed_result = COMPARE.compare(
            reference, changed, COMPARE.DEFAULT_POLICY
        )
        require(not changed_result["passed"],
                "materially different captures passed tolerance")
        require(
            changed_result["code"] == "RFX-XPLAT-PIXEL-TOLERANCE-001",
            "changed capture returned the wrong failure code",
        )


if __name__ == "__main__":
    main()
