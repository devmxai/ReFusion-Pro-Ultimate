#!/usr/bin/env python3
"""Write one schema-bound ReFusion visual qualification host receipt."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re


ROOT = pathlib.Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests" / "fixtures" / "render-plan" / "xplat-visual-v1"


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def key_values(path: pathlib.Path) -> dict[str, str]:
    result: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            result[key.strip()] = value.strip()
    return result


def render_plan_digests(path: pathlib.Path) -> list[str]:
    result: list[str] = []
    pattern = re.compile(
        r"^[0-9]+\s+(rfx-render-plan-v[0-9]+-fnv1a64:[0-9a-f]{16})$"
    )
    for line in path.read_text(encoding="utf-8").splitlines():
        match = pattern.fullmatch(line.strip())
        if match:
            result.append(match.group(1))
    if not result:
        raise ValueError("RenderPlan receipt contains no digests")
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--profile", required=True)
    parser.add_argument("--os", required=True)
    parser.add_argument("--architecture", required=True)
    parser.add_argument("--gpu", required=True)
    parser.add_argument("--driver", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--ninja", required=True)
    parser.add_argument("--skia-revision", required=True)
    parser.add_argument("--font-layout-digest", required=True)
    parser.add_argument("--capture", type=pathlib.Path, required=True)
    parser.add_argument("--suite", required=True)
    parser.add_argument("--passed", type=int, required=True)
    parser.add_argument("--failed", type=int, required=True)
    parser.add_argument("--not-run", action="append", default=[])
    parser.add_argument("--physically-run", action="store_true")
    parser.add_argument("--semantic-match", action="store_true")
    parser.add_argument("--visual-tolerance", action="store_true")
    parser.add_argument("--performance-qualified", action="store_true")
    parser.add_argument("--qualified", action="store_true")
    arguments = parser.parse_args()

    if not re.fullmatch(r"[0-9a-f]{40}", arguments.source_commit):
        raise ValueError("source commit must be a full lowercase Git object ID")
    if arguments.passed < 0 or arguments.failed < 0:
        raise ValueError("test counts cannot be negative")
    states = [
        True,
        arguments.physically_run,
        arguments.semantic_match,
        arguments.visual_tolerance,
        arguments.performance_qualified,
    ]
    for index, value in enumerate(states[1:], start=1):
        if value and not all(states[:index]):
            raise ValueError("qualification flags skip a required prior state")
    if arguments.qualified != all(states):
        raise ValueError("qualified must equal all preceding evidence states")
    if not arguments.capture.is_file():
        raise ValueError("qualification capture is missing")
    try:
        capture_artifact = arguments.capture.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        capture_artifact = arguments.capture.as_posix()

    project = key_values(FIXTURE / "expected-project.txt")
    color = json.loads(
        (ROOT / "contracts" / "visual" / "desktop-v1-sdr-color.json")
        .read_text(encoding="utf-8")
    )
    canonical_sha = project["canonical_sha256"]
    if canonical_sha.startswith("sha256:"):
        canonical_sha = canonical_sha.removeprefix("sha256:")
    receipt = {
        "schema": "refusion.xplat-visual-qualification-receipt.v1",
        "source_commit": arguments.source_commit,
        "profile": arguments.profile,
        "host": {
            "os": arguments.os,
            "architecture": arguments.architecture,
            "gpu": arguments.gpu,
            "driver": arguments.driver,
        },
        "toolchain": {
            "compiler": arguments.compiler,
            "cmake": arguments.cmake,
            "ninja": arguments.ninja,
            "skia_revision": arguments.skia_revision,
        },
        "contracts": {
            "color_id": color["contract_id"],
            "color_sha256": color["canonical_sha256"],
            "font_layout_digest": arguments.font_layout_digest,
        },
        "conformance": {
            "canonical_project_sha256": canonical_sha,
            "project_semantic_digest": project["snapshot_digest"],
            "property_registry_digest": project["registry_digest"],
            "contribution_registry_digest": project[
                "contribution_registry_digest"
            ],
            "command_receipt_sha256": sha256(
                FIXTURE / "expected-commands.txt"
            ),
            "font_layout_receipt_sha256": sha256(
                ROOT / "tests" / "fixtures" / "fonts" / "xplat-noto-v1"
                / "expected-layout.txt"
            ),
            "render_plan_digests": render_plan_digests(
                FIXTURE / "expected-render-plan.txt"
            ),
        },
        "capture": {
            "artifact": capture_artifact,
            "sha256": sha256(arguments.capture),
            "width": 640,
            "height": 360,
            "project_time_ns": 1_000_000_000,
            "preview_offscreen_exact": True,
            "comparison_policy": (
                "refusion.xplat-pixel-tolerance.desktop-v1.v1"
            ),
        },
        "tests": {
            "suite": arguments.suite,
            "passed": arguments.passed,
            "failed": arguments.failed,
            "not_run": arguments.not_run,
        },
        "claim_boundary": {
            "compiled": True,
            "physically_run": arguments.physically_run,
            "semantic_match": arguments.semantic_match,
            "visual_tolerance": arguments.visual_tolerance,
            "performance_qualified": arguments.performance_qualified,
            "qualified": arguments.qualified,
        },
    }
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(
        json.dumps(receipt, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
