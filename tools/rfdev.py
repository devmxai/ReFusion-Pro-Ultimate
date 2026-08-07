#!/usr/bin/env python3
"""Small, dependency-free Repo OS helper for ReFusion G0."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
STATUS = ROOT / "docs" / "status" / "CURRENT.md"


REQUIRED = [
    "AGENTS.md",
    "README.md",
    "docs/plans/MASTER_PLAN.md",
    "docs/status/CURRENT.md",
    "docs/product/PRODUCT_CONTRACT.md",
    "docs/architecture/INVARIANTS.md",
    "deps/manifest.lock.json",
    "deps/policies/qt-modules.json",
    "docs/legal/QT_DISTRIBUTION_GATE.md",
    "docs/product/MEDIA_MATRIX.md",
]


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def field(text: str, name: str, default: str = "unknown") -> str:
    match = re.search(rf"(?m)^{re.escape(name)}:\s*([^\n]+)$", text)
    return match.group(1).strip() if match else default


def current() -> dict[str, str]:
    text = STATUS.read_text(encoding="utf-8")
    return {
        "master_plan": field(text, "master_plan"),
        "master_plan_status": field(text, "master_plan_status"),
        "current_gate": field(text, "current_gate"),
        "gate_status": field(text, "gate_status"),
        "baseline_commit": field(text, "baseline_commit"),
        "last_checkpoint": field(text, "last_checkpoint"),
    }


def stage_path(gate: str) -> pathlib.Path:
    matches = sorted((ROOT / "docs" / "plans" / "stages").glob(f"{gate}-*/PLAN.md"))
    if len(matches) != 1:
        raise RuntimeError(f"expected one stage plan for {gate}, found {len(matches)}")
    return matches[0]


def command_status() -> int:
    print(json.dumps(current(), indent=2))
    return 0


def command_context() -> int:
    state = current()
    paths = [
        ROOT / "AGENTS.md",
        STATUS,
        ROOT / "docs" / "plans" / "MASTER_PLAN.md",
        stage_path(state["current_gate"]),
        ROOT / "docs" / "architecture" / "INVARIANTS.md",
    ]
    manifest = [{
        "path": str(path.relative_to(ROOT)),
        "sha256": sha256(path),
        "reason": "active authority context",
    } for path in paths]
    print(json.dumps({"state": state, "read_set": manifest}, indent=2))
    return 0


def command_docs_doctor() -> int:
    problems: list[str] = []
    for relative in REQUIRED:
        path = ROOT / relative
        if not path.is_file() or not path.read_text(encoding="utf-8").strip():
            problems.append(f"missing or empty: {relative}")

    ids: dict[str, str] = {}
    for path in (ROOT / "docs").rglob("*.md"):
        if "research" in path.parts:
            continue
        text = path.read_text(encoding="utf-8")
        doc_id = field(text, "id", "")
        if doc_id:
            relative = str(path.relative_to(ROOT))
            if doc_id in ids:
                problems.append(f"duplicate id {doc_id}: {ids[doc_id]} and {relative}")
            ids[doc_id] = relative

    try:
        state = current()
        stage_path(state["current_gate"])
    except RuntimeError as error:
        problems.append(str(error))

    result = {"checked_documents": len(ids), "problems": problems}
    print(json.dumps(result, indent=2))
    return 1 if problems else 0


def command_architecture_check() -> int:
    forbidden_headers = (
        "QtCore/", "QtGui/", "QtQuick/", "QtMultimedia/", "QImage", "QPainter",
        "QPixmap", "QPrinter", "QVideoFrame", "SkImage", "SkSurface",
        "Metal/", "VideoToolbox/", "CoreVideo/", "d3d11.h", "d3d12.h",
        "dxgi", "mfapi.h", "vulkan/", "media/NdkMediaCodec.h",
    )
    roots = [
        ROOT / "src" / "core",
        ROOT / "src" / "application",
        ROOT / "src" / "runtime",
    ]
    problems: list[str] = []
    checked = 0
    for base in roots:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in {".h", ".hpp", ".c", ".cc", ".cpp", ".ixx"}:
                continue
            checked += 1
            text = path.read_text(encoding="utf-8")
            for token in forbidden_headers:
                if token in text:
                    problems.append(f"{path.relative_to(ROOT)} contains forbidden boundary token {token}")

    for base, boundary_tokens in (
        (ROOT / "src" / "platform", ("include/core/Sk", "include/gpu/", "Qt6::")),
        (ROOT / "src" / "adapters" / "skia", ("QtCore/", "QtGui/", "QtQuick/")),
    ):
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in {".h", ".hpp", ".c", ".cc", ".cpp", ".m", ".mm", ".ixx"}:
                continue
            checked += 1
            text = path.read_text(encoding="utf-8")
            for token in boundary_tokens:
                if token in text:
                    problems.append(
                        f"{path.relative_to(ROOT)} crosses its boundary with token {token}"
                    )

    studio_forbidden_apis = (
        "QImage", "QPixmap", "QPainter", "QPrinter", "QVideoFrame",
        "QVideoSink", "QMediaPlayer", "QQuickPaintedItem",
    )
    for base in (ROOT / "apps" / "studio", ROOT / "src" / "studio"):
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in {".h", ".hpp", ".c", ".cc", ".cpp", ".ixx", ".qml"}:
                continue
            checked += 1
            text = path.read_text(encoding="utf-8")
            for token in studio_forbidden_apis:
                if token in text:
                    problems.append(f"{path.relative_to(ROOT)} contains forbidden Studio API {token}")

    qt_policy_path = ROOT / "deps" / "policies" / "qt-modules.json"
    qt_policy = json.loads(qt_policy_path.read_text(encoding="utf-8"))
    allowed_qt = set(qt_policy["allowed_studio_runtime_targets"])
    allowed_qt.update(qt_policy["conditional_targets"].keys())
    forbidden_qt = set(qt_policy["forbidden_product_targets"])
    linked_qt: set[str] = set()
    for path in ROOT.rglob("CMakeLists.txt"):
        if "out" in path.parts:
            continue
        text = path.read_text(encoding="utf-8")
        targets = set(re.findall(r"Qt6::[A-Za-z0-9_]+", text))
        linked_qt.update(targets)
        for target in targets & forbidden_qt:
            problems.append(f"{path.relative_to(ROOT)} links forbidden Qt target {target}")
        if path.is_relative_to(ROOT / "apps" / "studio"):
            for target in targets - allowed_qt:
                problems.append(f"{path.relative_to(ROOT)} links unlisted Qt target {target}")
        elif targets:
            problems.append(f"{path.relative_to(ROOT)} links Qt outside the Studio boundary: {sorted(targets)}")

    print(json.dumps({
        "checked_source_files": checked,
        "linked_qt_targets": sorted(linked_qt),
        "problems": problems,
    }, indent=2))
    return 1 if problems else 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("status", "context", "docs-doctor", "architecture-check"))
    args = parser.parse_args()
    try:
        return {
            "status": command_status,
            "context": command_context,
            "docs-doctor": command_docs_doctor,
            "architecture-check": command_architecture_check,
        }[args.command]()
    except (OSError, RuntimeError, UnicodeError) as error:
        print(f"rfdev error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
