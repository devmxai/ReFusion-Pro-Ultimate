#!/usr/bin/env python3
"""Small, dependency-free Repo OS helper for ReFusion G0."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
STATUS = ROOT / "docs" / "status" / "CURRENT.md"
DECISION_REGISTER = ROOT / "docs" / "decisions" / "REGISTER.md"
WORK_PACKAGES = ROOT / "docs" / "plans" / "stages"


REQUIRED = [
    "AGENTS.md",
    "README.md",
    "docs/plans/MASTER_PLAN.md",
    "docs/status/CURRENT.md",
    "docs/product/PRODUCT_CONTRACT.md",
    "docs/architecture/INVARIANTS.md",
    "docs/architecture/CROSS_PLATFORM_POLICY.md",
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


def list_field(text: str, name: str) -> list[str]:
    match = re.search(
        rf"(?m)^{re.escape(name)}:\s*\n((?:[ \t]+-[^\n]*\n?)*)", text
    )
    if not match:
        return []
    return [
        item.strip()
        for item in re.findall(r"(?m)^[ \t]+-\s*([^\n]+)$", match.group(1))
    ]


def frontmatter(path: pathlib.Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---\n"):
        return {}
    end = text.find("\n---\n", 4)
    if end == -1:
        return {}
    result: dict[str, str] = {}
    for line in text[4:end].splitlines():
        if ":" not in line or line.startswith((" ", "\t")):
            continue
        key, value = line.split(":", 1)
        result[key.strip()] = value.strip()
    return result


def git_output(*arguments: str) -> str:
    completed = subprocess.run(
        ["git", *arguments], cwd=ROOT, check=False, text=True, capture_output=True
    )
    if completed.returncode:
        detail = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"git {' '.join(arguments)} failed: {detail}")
    return completed.stdout.strip()


def current() -> dict[str, str | list[str]]:
    text = STATUS.read_text(encoding="utf-8")
    return {
        "master_plan": field(text, "master_plan"),
        "master_plan_status": field(text, "master_plan_status"),
        "current_gate": field(text, "current_gate"),
        "gate_status": field(text, "gate_status"),
        "baseline_commit": field(text, "baseline_commit"),
        "last_green_commit": field(text, "last_green_commit"),
        "last_checkpoint": field(text, "last_checkpoint"),
        "active_work_packages": list_field(text, "active_work_packages"),
        "blocking_risks": list_field(text, "blocking_risks"),
    }


def stage_path(gate: str) -> pathlib.Path:
    matches = sorted((ROOT / "docs" / "plans" / "stages").glob(f"{gate}-*/PLAN.md"))
    if len(matches) != 1:
        raise RuntimeError(f"expected one stage plan for {gate}, found {len(matches)}")
    return matches[0]


def work_package_path(work_package: str) -> pathlib.Path:
    matches = sorted(WORK_PACKAGES.rglob(f"{work_package}*.md"))
    if len(matches) != 1:
        raise RuntimeError(
            f"expected one work package file for {work_package}, found {len(matches)}"
        )
    return matches[0]


def checkpoint_path(checkpoint: str) -> pathlib.Path:
    matches = sorted((ROOT / "docs" / "status" / "checkpoints").glob(f"{checkpoint}.md"))
    if len(matches) != 1:
        raise RuntimeError(
            f"expected one checkpoint file for {checkpoint}, found {len(matches)}"
        )
    return matches[0]


def command_status() -> int:
    state = current()
    state["head"] = git_output("rev-parse", "HEAD")
    state["worktree_clean"] = not bool(git_output("status", "--porcelain"))
    print(json.dumps(state, indent=2))
    return 0


def command_context() -> int:
    state = current()
    gate = str(state["current_gate"])
    paths = [
        ROOT / "AGENTS.md",
        STATUS,
        ROOT / "docs" / "plans" / "MASTER_PLAN.md",
        stage_path(gate),
        ROOT / "docs" / "architecture" / "INVARIANTS.md",
        ROOT / "docs" / "architecture" / "CROSS_PLATFORM_POLICY.md",
        ROOT / "docs" / "product" / "PRODUCT_CONTRACT.md",
        ROOT / "docs" / "status" / "RISKS.md",
        checkpoint_path(str(state["last_checkpoint"])),
    ]
    paths.extend(
        work_package_path(work_package)
        for work_package in state["active_work_packages"]
    )
    unique_paths = list(dict.fromkeys(paths))
    manifest = [{
        "path": str(path.relative_to(ROOT)),
        "sha256": sha256(path),
        "reason": "active authority context",
    } for path in unique_paths]
    state["head"] = git_output("rev-parse", "HEAD")
    print(json.dumps({"state": state, "read_set": manifest}, indent=2))
    return 0


def render_decision_register() -> str:
    entries: list[dict[str, str]] = []
    for directory in (
        ROOT / "docs" / "decisions" / "adrs",
        ROOT / "docs" / "decisions" / "rfcs",
    ):
        for path in sorted(directory.glob("*.md")):
            metadata = frontmatter(path)
            if not metadata.get("id"):
                continue
            entries.append({
                "id": metadata["id"],
                "kind": metadata.get("kind", "unknown"),
                "status": metadata.get("status", "unknown"),
                "title": metadata.get("title", path.stem),
                "owner": metadata.get("owner_role", "unassigned"),
                "due": metadata.get("decision_due", "unspecified"),
                "path": str(path.relative_to(ROOT / "docs" / "decisions")),
            })
    lines = [
        "<!-- Generated by tools/rfdev.py decisions-register; do not edit. -->",
        "# Decision register",
        "",
        "| ID | Kind | Status | Title | Owner | Due | Source |",
        "|---|---|---|---|---|---|---|",
    ]
    for entry in sorted(entries, key=lambda item: item["id"]):
        lines.append(
            "| [{id}]({path}) | {kind} | {status} | {title} | {owner} | {due} | `{path}` |".format(
                **entry
            )
        )
    return "\n".join(lines) + "\n"


def command_decisions_register() -> int:
    DECISION_REGISTER.write_text(render_decision_register(), encoding="utf-8")
    print(json.dumps({
        "register": str(DECISION_REGISTER.relative_to(ROOT)),
        "sha256": sha256(DECISION_REGISTER),
    }, indent=2))
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
        stage_path(str(state["current_gate"]))
        checkpoint = checkpoint_path(str(state["last_checkpoint"]))
        checkpoint_metadata = frontmatter(checkpoint)
        checkpoint_commit = checkpoint_metadata.get("commit", "")
        if not checkpoint_commit:
            problems.append("current checkpoint has no commit")
        else:
            git_output("merge-base", "--is-ancestor", checkpoint_commit, "HEAD")
        for work_package in state["active_work_packages"]:
            path = work_package_path(work_package)
            metadata = frontmatter(path)
            if metadata.get("id") != work_package:
                problems.append(
                    f"work package ID mismatch: {work_package} -> {metadata.get('id', 'missing')}"
                )
            if metadata.get("status") not in {
                "active", "code-complete-awaiting-external-gate"
            }:
                problems.append(
                    f"active work package {work_package} has invalid status "
                    f"{metadata.get('status', 'missing')}"
                )
            evidence = metadata.get("evidence", "")
            if not evidence or not (ROOT / evidence).is_file():
                problems.append(
                    f"active work package {work_package} has no evidence record: {evidence}"
                )
    except RuntimeError as error:
        problems.append(str(error))

    invariant_metadata = frontmatter(ROOT / "docs" / "architecture" / "INVARIANTS.md")
    if invariant_metadata.get("status") != "accepted":
        problems.append("architecture invariants must be accepted before G0 exit")

    for path in sorted((ROOT / "docs" / "decisions" / "adrs").glob("*.md")):
        metadata = frontmatter(path)
        for required in ("id", "kind", "status", "title", "owner_role", "decision_due"):
            if not metadata.get(required):
                problems.append(
                    f"{path.relative_to(ROOT)} is missing decision metadata {required}"
                )

    expected_register = render_decision_register()
    if not DECISION_REGISTER.is_file():
        problems.append("decision register is missing; run rfdev.py decisions-register")
    elif DECISION_REGISTER.read_text(encoding="utf-8") != expected_register:
        problems.append("decision register is stale; run rfdev.py decisions-register")

    result = {"checked_documents": len(ids), "problems": problems}
    print(json.dumps(result, indent=2))
    return 1 if problems else 0


def studio_authority_problems(root: pathlib.Path) -> list[str]:
    problems: list[str] = []
    boundaries = (root / "apps" / "studio", root / "src" / "studio")
    forbidden_tokens = (
        "ProjectAuthority",
        'refusion/core/ProjectAuthority.hpp',
    )
    composition_only_tokens = (
        "create_platform_gpu_device_service",
        "create_platform_viewport_presenter",
        "SkiaGpuContexts::create",
    )
    for base in boundaries:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in {".h", ".hpp", ".c", ".cc", ".cpp", ".ixx", ".qml"}:
                continue
            text = path.read_text(encoding="utf-8")
            for token in forbidden_tokens:
                if token in text:
                    problems.append(
                        f"{path.relative_to(root)} owns forbidden concrete authority token {token}"
                    )
            if "composition" not in path.parts:
                for token in composition_only_tokens:
                    if token in text:
                        problems.append(
                            f"{path.relative_to(root)} constructs engine runtime "
                            f"outside composition with {token}"
                        )
    studio_cmake = root / "apps" / "studio" / "CMakeLists.txt"
    if studio_cmake.is_file() and "ReFusion::Core" in studio_cmake.read_text(encoding="utf-8"):
        problems.append("apps/studio/CMakeLists.txt links Core directly instead of Application")
    cli_cmake = root / "apps" / "cli" / "CMakeLists.txt"
    if cli_cmake.is_file() and "ReFusion::Core" in cli_cmake.read_text(encoding="utf-8"):
        problems.append("apps/cli/CMakeLists.txt links Core directly instead of Application")
    return problems


def cross_platform_contract_problems(root: pathlib.Path) -> list[str]:
    problems: list[str] = []
    platform_directive = re.compile(
        r"^\s*#\s*(?:if|ifdef|ifndef)\b[^\n]*"
        r"(?:_WIN32|\bWIN32\b|__APPLE__|__ANDROID__|\bANDROID\b|Q_OS_[A-Z0-9_]+)",
        re.MULTILINE,
    )
    common_roots = (
        root / "src" / "core",
        root / "src" / "application",
        root / "src" / "runtime",
        root / "apps" / "studio",
        root / "apps" / "cli",
    )
    for base in common_roots:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in {".h", ".hpp", ".c", ".cc", ".cpp", ".ixx"}:
                continue
            text = path.read_text(encoding="utf-8")
            if platform_directive.search(text):
                problems.append(
                    f"{path.relative_to(root)} contains a platform conditional outside an adapter"
                )

    presets_path = root / "CMakePresets.json"
    if not presets_path.is_file():
        problems.append("CMakePresets.json is missing the cross-platform build contract")
        return problems
    presets = json.loads(presets_path.read_text(encoding="utf-8"))
    configure_names = {
        value.get("name") for value in presets.get("configurePresets", [])
    }
    workflow_names = {
        value.get("name") for value in presets.get("workflowPresets", [])
    }
    required_lanes = {
        "macos-core", "macos-studio", "macos-graphics",
        "windows-core", "windows-studio", "windows-graphics",
    }
    for name in sorted(required_lanes - configure_names):
        problems.append(f"CMakePresets.json is missing configure lane {name}")
    for name in sorted(required_lanes - workflow_names):
        problems.append(f"CMakePresets.json is missing workflow lane {name}")
    return problems


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
    problems.extend(studio_authority_problems(ROOT))
    problems.extend(cross_platform_contract_problems(ROOT))
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
    parser.add_argument(
        "command",
        choices=(
            "status", "context", "docs-doctor", "architecture-check",
            "decisions-register",
        ),
    )
    args = parser.parse_args()
    try:
        return {
            "status": command_status,
            "context": command_context,
            "docs-doctor": command_docs_doctor,
            "architecture-check": command_architecture_check,
            "decisions-register": command_decisions_register,
        }[args.command]()
    except (OSError, RuntimeError, UnicodeError) as error:
        print(f"rfdev error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
