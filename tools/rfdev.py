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
VISUAL_BOUNDARY_EXCEPTIONS = (
    ROOT / "contracts" / "architecture"
    / "cross-platform-visual-boundary-exceptions.json"
)
VISUAL_CAPABILITY_MATRIX = (
    ROOT / "contracts" / "visual" / "cross-platform-capability-matrix.json"
)
VISUAL_COLOR_CONTRACT = (
    ROOT / "contracts" / "visual" / "desktop-v1-sdr-color.json"
)
VISUAL_PIXEL_TOLERANCE = (
    ROOT / "contracts" / "visual" / "desktop-v1-pixel-tolerance.json"
)
VISUAL_QUALIFICATION_RECEIPT_SCHEMA = (
    ROOT / "contracts" / "visual"
    / "xplat-qualification-receipt-v1.schema.json"
)

# Frozen by XPF-WP00A. The manifest keeps the human-readable baseline; this
# digest prevents silently adding or changing a baseline signature. Active
# allowances may only be removed or reduced.
FROZEN_VISUAL_BOUNDARY_BASELINE_SHA256 = (
    "b347493597edd70bbf42683bcb8485c9d8ad2042d29d19ee7205c6d2488d1e25"
)


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
    "contracts/visual/cross-platform-capability-matrix.json",
    "contracts/visual/desktop-v1-sdr-color.json",
    "contracts/visual/desktop-v1-pixel-tolerance.json",
    "contracts/visual/xplat-qualification-receipt-v1.schema.json",
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
        "active_guardrails": list_field(text, "active_guardrails"),
        "blocking_risks": list_field(text, "blocking_risks"),
    }


def document_path(document_id: str) -> pathlib.Path:
    matches: list[pathlib.Path] = []
    for path in (ROOT / "docs").rglob("*.md"):
        if "research" in path.parts:
            continue
        if frontmatter(path).get("id") == document_id:
            matches.append(path)
    if len(matches) != 1:
        raise RuntimeError(
            f"expected one document for {document_id}, found {len(matches)}"
        )
    return matches[0]


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
    paths.extend(
        document_path(guardrail)
        for guardrail in state["active_guardrails"]
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
                "path": path.relative_to(
                    ROOT / "docs" / "decisions"
                ).as_posix(),
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
        active_guardrails = list(state["active_guardrails"])
        if len(active_guardrails) != len(set(active_guardrails)):
            problems.append("CURRENT.md contains duplicate active_guardrails")
        for guardrail in active_guardrails:
            if guardrail not in ids:
                problems.append(f"unknown active guardrail: {guardrail}")
                continue
            guardrail_path = ROOT / ids[guardrail]
            metadata = frontmatter(guardrail_path)
            if metadata.get("status") != "active":
                problems.append(
                    f"active guardrail {guardrail} has invalid status "
                    f"{metadata.get('status', 'missing')}"
                )
            if metadata.get("master_plan") != state["master_plan"]:
                problems.append(
                    f"active guardrail {guardrail} master_plan mismatch: "
                    f"{metadata.get('master_plan', 'missing')}"
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

    backend_private_tokens = (
        "backend_private_device(",
        "backend_private_submission_queue(",
        "backend_private_target(",
        "backend_private_host(",
    )
    backend_private_contract_files = {
        "src/runtime/gpu/GpuDeviceService.cpp",
        "src/runtime/gpu/include/refusion/runtime/gpu/GpuDeviceService.hpp",
        "src/runtime/presentation/ViewportPresentation.cpp",
        "src/runtime/presentation/include/refusion/runtime/presentation/ViewportPresentation.hpp",
    }
    for base in common_roots:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in {".h", ".hpp", ".c", ".cc", ".cpp", ".ixx"}:
                continue
            relative = path.relative_to(root).as_posix()
            if relative in backend_private_contract_files:
                continue
            text = path.read_text(encoding="utf-8")
            for token in backend_private_tokens:
                if token in text:
                    problems.append(
                        f"{relative} accesses backend-private GPU state outside a native adapter"
                    )

    native_render_bindings = (
        root / "src" / "adapters" / "skia" / "SkiaGpuContextsMetal.mm",
        root / "src" / "adapters" / "skia" / "SkiaGpuContextsD3D12.cpp",
        root / "src" / "adapters" / "skia" / "SkiaGpuContextsVulkanCanary.cpp",
    )
    forbidden_native_semantics = (
        "refusion/core/",
        "ProjectDocument.hpp",
        "RenderPlanCompiler.hpp",
        "core::Project",
        "evaluate_visual_render_plan(",
        "draw_visual_render_plan(",
        "GaussianBlur",
        "DropShadow",
        "Glow",
        "BlendMode",
        "RoundedRectMask",
        "GradientStop",
    )
    forbidden_product_contexts = (
        "skgpu::graphite::ContextFactory",
        "include/gpu/graphite/Context.h",
    )
    for path in native_render_bindings:
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        for token in forbidden_native_semantics:
            if token in text:
                problems.append(
                    f"{path.relative_to(root)} owns forbidden native visual semantic {token}"
                )
        for token in forbidden_product_contexts:
            if token in text:
                problems.append(
                    f"{path.relative_to(root)} creates an unqualified second product render context {token}"
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
        "macos-core", "macos-studio", "macos-graphics", "macos-visual",
        "macos-media", "windows-core", "windows-studio", "windows-graphics",
        "windows-visual", "windows-media", "ios-core-canary",
        "ios-graphics-canary", "android-core-canary",
        "android-graphics-canary",
    }
    for name in sorted(required_lanes - configure_names):
        problems.append(f"CMakePresets.json is missing configure lane {name}")
    for name in sorted(required_lanes - workflow_names):
        problems.append(f"CMakePresets.json is missing workflow lane {name}")
    return problems


def mobile_contract_canary_problems(root: pathlib.Path) -> list[str]:
    problems: list[str] = []
    required_text = {
        "src/platform/apple/metal_ios/IosMetalContractCanary.mm": (
            "RFX-IOS-CANARY-NOT-PRODUCT", "NativeWindowSystem::ui_view",
        ),
        "src/platform/android/vulkan/AndroidVulkanContractCanary.cpp": (
            "RFX-ANDROID-CANARY-NOT-PRODUCT",
            "NativeWindowSystem::android_native_window",
        ),
        "src/adapters/skia/SkiaGpuContextsVulkanCanary.cpp": (
            "RFX-ANDROID-CANARY-NOT-PRODUCT", "GrVkBackendContext.h",
        ),
        "deps/profiles/skia/ios-arm64-metal-canary.gn": (
            'target_os = "ios"', 'skia_use_metal = true',
        ),
        "deps/profiles/skia/android-arm64-vulkan-canary.gn": (
            'target_os = "android"', 'skia_use_vulkan = true',
            'getenv("ANDROID_NDK_HOME")',
        ),
        ".github/workflows/mobile-contract-canaries.yml": (
            "ios-core-canary", "ios-graphics-canary",
            "android-core-canary", "android-graphics-canary",
        ),
    }
    for relative, tokens in required_text.items():
        path = root / relative
        if not path.is_file():
            problems.append(f"mobile contract canary file is missing: {relative}")
            continue
        text = path.read_text(encoding="utf-8")
        for token in tokens:
            if token not in text:
                problems.append(
                    f"mobile contract canary {relative} is missing {token}"
                )

    ios_source = root / "src/platform/apple/metal_ios/IosMetalContractCanary.mm"
    if ios_source.is_file() and "AppKit" in ios_source.read_text(encoding="utf-8"):
        problems.append("iOS contract canary imports desktop AppKit")
    return problems


def visual_color_contract_problems(root: pathlib.Path) -> list[str]:
    path = root / "contracts" / "visual" / "desktop-v1-sdr-color.json"
    if not path.is_file():
        return ["Desktop v1 SDR color contract is missing"]
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, UnicodeError) as error:
        return [f"invalid Desktop v1 SDR color contract: {error}"]

    problems: list[str] = []
    expected = {
        "schema_version": 1,
        "contract_id": "refusion.color.desktop-v1-sdr.v1",
        "decision": "ADR-0010",
        "authored_encoding": "srgb-unorm8",
        "primaries": "rec709-d65",
        "transfer": "srgb",
        "project_alpha": "straight",
        "compositing_alpha": "premultiplied",
        "blend_filter_working_space": "srgb-encoded",
        "gradient_interpolation": "srgb-straight",
        "filter_edge": "transparent-decal",
        "target_format": "bgra8-unorm",
        "output_transfer": "srgb",
    }
    for key, value in expected.items():
        if payload.get(key) != value:
            problems.append(
                f"Desktop v1 SDR color contract {key} mismatch: "
                f"{payload.get(key)!r} != {value!r}"
            )
    if payload.get("status") not in {"proposed", "accepted"}:
        problems.append("Desktop v1 SDR color contract status is invalid")
    canonical_keys = [
        "contract_id", "schema_version", "authored_encoding", "primaries",
        "transfer", "project_alpha", "compositing_alpha",
        "blend_filter_working_space", "gradient_interpolation", "filter_edge",
        "target_format", "output_transfer",
    ]
    canonical_names = {"contract_id": "profile_id"}
    canonical = "".join(
        f"{canonical_names.get(key, key)}={payload.get(key)}\n"
        for key in canonical_keys
    ).encode("ascii", errors="strict")
    actual_digest = hashlib.sha256(canonical).hexdigest()
    if payload.get("canonical_sha256") != actual_digest:
        problems.append(
            "Desktop v1 SDR color canonical digest mismatch: "
            f"{payload.get('canonical_sha256')} != {actual_digest}"
        )

    required_source_tokens = {
        "src/core/ColorContract.cpp": (
            "refusion.color.desktop-v1-sdr.v1",
            "desktop_v1_sdr_color_contract_digest",
        ),
        "src/runtime/render/RenderPlanCompiler.cpp": (
            "plan.color_contract.profile_id",
            "plan.color_contract_digest",
        ),
        "src/adapters/skia/SkiaSceneCompositor.cpp": (
            "RFX-COLOR-CONTRACT-001",
            "desktop_v1_sdr_color_contract_digest",
        ),
    }
    for relative, tokens in required_source_tokens.items():
        source = root / relative
        if not source.is_file():
            problems.append(f"color contract source is missing: {relative}")
            continue
        text = source.read_text(encoding="utf-8")
        for token in tokens:
            if token not in text:
                problems.append(
                    f"color contract source {relative} is missing {token}"
                )
    return problems


def visual_qualification_contract_problems(root: pathlib.Path) -> list[str]:
    tolerance_path = (
        root / "contracts" / "visual" / "desktop-v1-pixel-tolerance.json"
    )
    receipt_path = (
        root / "contracts" / "visual"
        / "xplat-qualification-receipt-v1.schema.json"
    )
    problems: list[str] = []
    for path, label in (
        (tolerance_path, "Desktop v1 pixel tolerance"),
        (receipt_path, "cross-platform qualification receipt schema"),
    ):
        if not path.is_file():
            problems.append(f"{label} is missing")
            continue
        try:
            json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, UnicodeError) as error:
            problems.append(f"invalid {label}: {error}")
    if problems:
        return problems

    tolerance = json.loads(tolerance_path.read_text(encoding="utf-8"))
    expected_tolerance = {
        "schema_version": 1,
        "policy_id": "refusion.xplat-pixel-tolerance.desktop-v1.v1",
        "decision": "ADR-0010",
        "reference_profile": "macos-metal-desktop-v1",
        "candidate_profile": "windows-d3d12-desktop-v1",
        "capture_format": "rgb8-ppm-p6",
        "dimensions_must_match": True,
        "alpha_policy": "qualification-source-bgra-alpha-must-be-255",
    }
    for key, value in expected_tolerance.items():
        if tolerance.get(key) != value:
            problems.append(
                f"Desktop v1 pixel tolerance {key} mismatch: "
                f"{tolerance.get(key)!r} != {value!r}"
            )
    if tolerance.get("status") not in {"proposed", "accepted"}:
        problems.append("Desktop v1 pixel tolerance status is invalid")
    expected_metrics = {
        "maximum_channel_delta": 8,
        "mean_absolute_channel_delta": 0.75,
        "pixels_over_delta_3_ratio": 0.005,
        "minimum_ssim": 0.995,
    }
    if tolerance.get("metrics") != expected_metrics:
        problems.append("Desktop v1 pixel tolerance metrics mismatch")

    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    if receipt.get("$schema") != "https://json-schema.org/draft/2020-12/schema":
        problems.append("qualification receipt schema draft mismatch")
    properties = receipt.get("properties")
    if not isinstance(properties, dict):
        problems.append("qualification receipt properties are missing")
    else:
        required_properties = {
            "schema", "source_commit", "profile", "host", "toolchain",
            "contracts", "conformance", "capture", "tests",
            "claim_boundary",
        }
        if set(properties) != required_properties:
            problems.append("qualification receipt property set mismatch")
        schema_value = properties.get("schema", {})
        if schema_value.get("const") != (
            "refusion.xplat-visual-qualification-receipt.v1"
        ):
            problems.append("qualification receipt identity mismatch")
        capture = properties.get("capture", {}).get("properties", {})
        policy = capture.get("comparison_policy", {})
        if policy.get("const") != expected_tolerance["policy_id"]:
            problems.append("qualification receipt tolerance binding mismatch")

    required_source_tokens = {
        "src/runtime/presentation/include/refusion/runtime/presentation/ViewportPresentation.hpp": (
            "VisualOutputConsumer", "interactive_preview",
        ),
        "src/adapters/skia/SkiaVisualProgramExecutor.cpp": (
            "prepare_visual_output_frame", "output_consumer",
        ),
        "tests/integration/skia_fixture_renderer_test.mm": (
            "offline_export", "offline_pixels == pixels",
            "REFUSION_XPLAT_CAPTURE_PPM",
        ),
        "tests/integration/d3d12_fixture_renderer_test.cpp": (
            "offline_export", "offline_pixels == preview_pixels",
            "REFUSION_XPLAT_CAPTURE_PPM", "WaitForSingleObject",
        ),
        "tools/qualification/compare_visual_captures.py": (
            "maximum_channel_delta", "pixels_over_delta_3_ratio",
            "minimum_ssim", "RFX-XPLAT-PIXEL-TOLERANCE-001",
        ),
        "tools/qualification/write_visual_qualification_receipt.py": (
            "refusion.xplat-visual-qualification-receipt.v1",
            "canonical_project_sha256", "contribution_registry_digest",
            "preview_offscreen_exact",
        ),
    }
    for relative, tokens in required_source_tokens.items():
        source = root / relative
        if not source.is_file():
            problems.append(f"qualification source is missing: {relative}")
            continue
        text = source.read_text(encoding="utf-8")
        for token in tokens:
            if token not in text:
                problems.append(
                    f"qualification source {relative} is missing {token}"
                )
    return problems


def windows_bringup_contract_problems(root: pathlib.Path) -> list[str]:
    problems: list[str] = []
    required_text = {
        ".github/workflows/windows-graphics.yml": (
            "windows-2025", "Invoke-ReFusionWindowsBringup.ps1",
            "-Lane Graphics", "-FreshDependencies", "-CompileOnly",
        ),
        "tools/windows/Invoke-ReFusionWindowsBringup.ps1": (
            "windows-core", "windows-graphics", "windows-visual",
            "lock-skia-materialization", "windows-x64-d3d12",
            "qualifying_source", "qt_release_entitlement_checked",
            "refusion.d3d12_fixture_renderer",
            "compare_visual_captures.py",
            "write_visual_qualification_receipt.py", "execution_mode",
        ),
        "src/platform/windows/d3d12/DxgiViewportPresenter.cpp": (
            "kFenceWaitTimeoutMs", "RFX-D3D12-FENCE-TIMEOUT",
            "GetDeviceRemovedReason", "native_wait_timeouts",
        ),
    }
    for relative, tokens in required_text.items():
        path = root / relative
        if not path.is_file():
            problems.append(f"Windows bring-up contract file is missing: {relative}")
            continue
        text = path.read_text(encoding="utf-8")
        for token in tokens:
            if token not in text:
                problems.append(
                    f"Windows bring-up contract {relative} is missing {token}"
                )
    presenter = root / "src/platform/windows/d3d12/DxgiViewportPresenter.cpp"
    if presenter.is_file() and "INFINITE" in presenter.read_text(encoding="utf-8"):
        problems.append("DXGI presenter contains a forbidden unbounded wait")
    return problems


SOURCE_SUFFIXES = {".h", ".hpp", ".c", ".cc", ".cpp", ".m", ".mm", ".ixx"}
STUDIO_SOURCE_SUFFIXES = SOURCE_SUFFIXES | {".qml"}


def _source_files(base: pathlib.Path, suffixes: set[str]) -> list[pathlib.Path]:
    if not base.exists():
        return []
    return sorted(
        path for path in base.rglob("*")
        if path.is_file() and path.suffix in suffixes
    )


def _add_visual_finding(
    findings: dict[tuple[str, str, str], int],
    root: pathlib.Path,
    path: pathlib.Path,
    rule: str,
    symbol: str,
    count: int = 1,
) -> None:
    relative = path.relative_to(root).as_posix()
    key = (rule, relative, symbol)
    findings[key] = findings.get(key, 0) + count


def _is_native_visual_source(root: pathlib.Path, path: pathlib.Path) -> bool:
    relative = path.relative_to(root).as_posix()
    if relative.startswith("src/platform/"):
        return True
    if relative.startswith("src/adapters/skia/system_fonts/"):
        return True
    if relative == (
        "src/adapters/skia/include/refusion/adapters/skia/SkiaGpuContexts.hpp"
    ):
        return True
    if not relative.startswith("src/adapters/skia/"):
        return False
    name = path.name.lower()
    return any(token in name for token in ("metal", "d3d", "dxgi", "vulkan"))


def collect_visual_boundary_findings(root: pathlib.Path) -> list[dict[str, object]]:
    """Collect stable, count-based XPF visual-boundary violations."""
    findings: dict[tuple[str, str, str], int] = {}
    core_symbol = re.compile(r"(?<![A-Za-z0-9_])(?:refusion::)?core::([A-Za-z_][A-Za-z0-9_]*)")

    native_bases = (root / "src" / "platform", root / "src" / "adapters" / "skia")
    for base in native_bases:
        for path in _source_files(base, SOURCE_SUFFIXES):
            if not _is_native_visual_source(root, path):
                continue
            text = path.read_text(encoding="utf-8")
            project_include_count = text.count("ProjectDocument.hpp")
            if project_include_count:
                _add_visual_finding(
                    findings, root, path, "native-project-semantics",
                    "ProjectDocument.hpp", project_include_count,
                )
            symbols: dict[str, int] = {}
            for match in core_symbol.finditer(text):
                symbol = f"core::{match.group(1)}"
                symbols[symbol] = symbols.get(symbol, 0) + 1
            for symbol, count in symbols.items():
                _add_visual_finding(
                    findings, root, path, "native-project-semantics", symbol, count
                )

    common_bases = (
        root / "src" / "core",
        root / "src" / "application",
        root / "src" / "runtime",
        root / "src" / "adapters" / "skia",
    )
    platform_macro = re.compile(r"\b(__APPLE__|_WIN32|__ANDROID__|ANDROID)\b")
    preprocessor_condition = re.compile(r"^\s*#\s*(?:if|ifdef|ifndef|elif)\b")
    include_pattern = re.compile(r"^\s*#\s*(?:include|import)\s*[<\"]([^>\"]+)", re.MULTILINE)
    forbidden_common_include = re.compile(
        r"^(?:Metal/|AppKit/|QuartzCore/|CoreText/|d3d|dxgi|windows\.h$|"
        r"vulkan/|android/|media/Ndk|.*SkFontMgr_mac_ct\.h$|"
        r".*SkTypeface_win\.h$|.*SkFontMgr_android\.h$)",
        re.IGNORECASE,
    )
    for base in common_bases:
        for path in _source_files(base, SOURCE_SUFFIXES):
            if _is_native_visual_source(root, path):
                continue
            text = path.read_text(encoding="utf-8")
            macro_counts: dict[str, int] = {}
            for line in text.splitlines():
                if not preprocessor_condition.search(line):
                    continue
                for match in platform_macro.finditer(line):
                    macro = match.group(1)
                    macro_counts[macro] = macro_counts.get(macro, 0) + 1
            for macro, count in macro_counts.items():
                _add_visual_finding(
                    findings, root, path, "common-platform-branch", macro, count
                )
            include_counts: dict[str, int] = {}
            for match in include_pattern.finditer(text):
                include = match.group(1)
                if forbidden_common_include.search(include):
                    include_counts[include] = include_counts.get(include, 0) + 1
            for include, count in include_counts.items():
                _add_visual_finding(
                    findings, root, path, "common-native-include", include, count
                )

    skia_cmake = root / "src" / "adapters" / "skia" / "CMakeLists.txt"
    if skia_cmake.is_file():
        text = skia_cmake.read_text(encoding="utf-8")
        common_link = re.compile(
            r"target_link_libraries\s*\(\s*refusion_skia_adapter\b"
            r"(?P<body>.*?)\)",
            re.DOTALL,
        )
        for block in common_link.finditer(text):
            for dependency in re.findall(r"ReFusion::Platform[A-Za-z0-9_]*", block.group("body")):
                _add_visual_finding(
                    findings, root, skia_cmake, "common-native-link", dependency
                )
        common_definition = re.compile(
            r"target_compile_definitions\s*\(\s*refusion_skia_adapter\b"
            r"(?P<body>.*?)\)",
            re.DOTALL,
        )
        for block in common_definition.finditer(text):
            for definition in re.findall(
                r"REFUSION_SKIA_(?:APPLE|WINDOWS|ANDROID|METAL|D3D|VULKAN)[A-Z0-9_]*",
                block.group("body"),
            ):
                _add_visual_finding(
                    findings, root, skia_cmake, "common-platform-definition", definition
                )

    effect_type = re.compile(
        r"\brefusion::core::([A-Za-z_][A-Za-z0-9_]*(?:Effect|Mask))\b"
    )
    add_effect = re.compile(r"addSelectedEffect\(\"([^\"]+)\"\)")
    cpp_effect_dispatch = re.compile(
        r"\beffect_kind\s*==\s*QStringLiteral\(\"([^\"]+)\"\)"
    )
    qml_kind_dispatch = re.compile(
        r"\bmodelData\.kind\s*(?:===|!==)\s*\"([^\"]+)\""
    )
    generic_value_kinds = {"boolean", "color", "enum", "number", "paint", "string"}
    for base in (root / "apps" / "studio", root / "src" / "studio"):
        for path in _source_files(base, STUDIO_SOURCE_SUFFIXES):
            text = path.read_text(encoding="utf-8")
            symbols: dict[str, int] = {}
            for match in effect_type.finditer(text):
                symbol = f"core::{match.group(1)}"
                symbols[symbol] = symbols.get(symbol, 0) + 1
            for match in add_effect.finditer(text):
                symbol = f"add-effect:{match.group(1)}"
                symbols[symbol] = symbols.get(symbol, 0) + 1
            for match in cpp_effect_dispatch.finditer(text):
                symbol = f"effect-dispatch:{match.group(1)}"
                symbols[symbol] = symbols.get(symbol, 0) + 1
            for match in qml_kind_dispatch.finditer(text):
                if match.group(1) in generic_value_kinds:
                    continue
                symbol = f"qml-kind-dispatch:{match.group(1)}"
                symbols[symbol] = symbols.get(symbol, 0) + 1
            for symbol, count in symbols.items():
                _add_visual_finding(
                    findings, root, path, "studio-effect-vocabulary", symbol, count
                )

    return [
        {"rule": rule, "path": path, "symbol": symbol, "occurrences": count}
        for (rule, path, symbol), count in sorted(findings.items())
    ]


def cross_platform_visual_boundary_problems(
    root: pathlib.Path,
    manifest_path: pathlib.Path | None = None,
    enforce_frozen_baseline: bool | None = None,
) -> list[str]:
    findings = collect_visual_boundary_findings(root)
    problems: list[str] = []
    if manifest_path is None:
        manifest_path = (
            VISUAL_BOUNDARY_EXCEPTIONS
            if root.resolve() == ROOT.resolve()
            else root / "contracts" / "architecture"
            / "cross-platform-visual-boundary-exceptions.json"
        )
    if enforce_frozen_baseline is None:
        enforce_frozen_baseline = root.resolve() == ROOT.resolve()
    if not manifest_path.is_file():
        return [
            f"visual-boundary exception manifest is missing: {manifest_path}",
            *[
                "unapproved visual-boundary violation: "
                f"{item['rule']} {item['path']} {item['symbol']} "
                f"occurrences={item['occurrences']}"
                for item in findings
            ],
        ]

    try:
        payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, UnicodeError) as error:
        return [f"invalid visual-boundary exception manifest: {error}"]
    if payload.get("schema_version") != 1:
        problems.append("visual-boundary exception manifest schema_version must be 1")
    if payload.get("policy_id") != "PLAN-XPLAT-FIX-001/XPF-WP00A":
        problems.append("visual-boundary exception manifest policy_id mismatch")
    if payload.get("mode") != "shrink-only":
        problems.append("visual-boundary exception manifest mode must be shrink-only")

    baseline = payload.get("frozen_baseline", [])
    if not isinstance(baseline, list):
        return ["visual-boundary frozen_baseline must be an array"]
    baseline_digest = hashlib.sha256(
        json.dumps(
            baseline, sort_keys=True, separators=(",", ":"), ensure_ascii=True
        ).encode("utf-8")
    ).hexdigest()
    if (
        enforce_frozen_baseline
        and baseline_digest != FROZEN_VISUAL_BOUNDARY_BASELINE_SHA256
    ):
        problems.append(
            "visual-boundary frozen baseline changed: "
            f"{baseline_digest} != {FROZEN_VISUAL_BOUNDARY_BASELINE_SHA256}"
        )

    baseline_by_id: dict[str, dict[str, object]] = {}
    baseline_by_key: dict[tuple[str, str, str], dict[str, object]] = {}
    required = {
        "id", "rule", "path", "symbol", "initial_occurrences",
        "owner", "removal_work_package", "expires",
    }
    for entry in baseline:
        if not isinstance(entry, dict):
            problems.append("visual-boundary baseline entry must be an object")
            continue
        missing = sorted(required - entry.keys())
        if missing:
            problems.append(f"visual-boundary baseline missing fields: {missing}")
            continue
        exception_id = str(entry["id"])
        if exception_id in baseline_by_id:
            problems.append(f"duplicate visual-boundary baseline id: {exception_id}")
            continue
        rule = str(entry["rule"])
        path = str(entry["path"])
        symbol = str(entry["symbol"])
        count = entry["initial_occurrences"]
        if any(character in path or character in symbol for character in "*?[]"):
            problems.append(f"visual-boundary baseline {exception_id} uses a wildcard")
        if not isinstance(count, int) or isinstance(count, bool) or count <= 0:
            problems.append(
                f"visual-boundary baseline {exception_id} has invalid initial_occurrences"
            )
            continue
        if not str(entry["owner"]).strip():
            problems.append(f"visual-boundary baseline {exception_id} has no owner")
        if not str(entry["removal_work_package"]).startswith("XPF-WP"):
            problems.append(
                f"visual-boundary baseline {exception_id} has invalid removal work package"
            )
        if not str(entry["expires"]).strip():
            problems.append(f"visual-boundary baseline {exception_id} has no expiry")
        key = (rule, path, symbol)
        if key in baseline_by_key:
            problems.append(
                f"duplicate visual-boundary baseline signature: {rule} {path} {symbol}"
            )
            continue
        baseline_by_id[exception_id] = entry
        baseline_by_key[key] = entry

    allowances = payload.get("active_allowances", {})
    if not isinstance(allowances, dict):
        problems.append("visual-boundary active_allowances must be an object")
        allowances = {}
    exceptions: dict[tuple[str, str, str], dict[str, object]] = {}
    for exception_id, count in allowances.items():
        entry = baseline_by_id.get(exception_id)
        if entry is None:
            problems.append(
                f"new visual-boundary allowance is forbidden: {exception_id}"
            )
            continue
        initial = entry["initial_occurrences"]
        if not isinstance(count, int) or isinstance(count, bool) or count <= 0:
            problems.append(
                f"visual-boundary allowance {exception_id} has invalid count"
            )
            continue
        if count > initial:
            problems.append(
                f"visual-boundary allowance grew: {exception_id} {count}>{initial}"
            )
        key = (str(entry["rule"]), str(entry["path"]), str(entry["symbol"]))
        exception = dict(entry)
        exception["allowed_occurrences"] = count
        exceptions[key] = exception

    actual = {
        (str(item["rule"]), str(item["path"]), str(item["symbol"])):
            int(item["occurrences"])
        for item in findings
    }
    for key, count in actual.items():
        entry = exceptions.get(key)
        if entry is None:
            problems.append(
                "unapproved visual-boundary violation: "
                f"{key[0]} {key[1]} {key[2]} occurrences={count}"
            )
            continue
        allowed = int(entry["allowed_occurrences"])
        if count > allowed:
            problems.append(
                f"visual-boundary violation grew: {entry['id']} {count}>{allowed}"
            )
        elif count < allowed:
            problems.append(
                f"visual-boundary exception must shrink: {entry['id']} "
                f"actual={count} allowed={allowed}"
            )
    for key, entry in exceptions.items():
        if key not in actual:
            problems.append(
                f"stale visual-boundary exception must be removed: {entry['id']}"
            )
    return problems


def visual_capability_matrix_problems(
    root: pathlib.Path,
    matrix_path: pathlib.Path | None = None,
) -> list[str]:
    if matrix_path is None:
        matrix_path = (
            VISUAL_CAPABILITY_MATRIX
            if root.resolve() == ROOT.resolve()
            else root / "contracts" / "visual"
            / "cross-platform-capability-matrix.json"
        )
    if not matrix_path.is_file():
        return [f"visual capability matrix is missing: {matrix_path}"]
    try:
        payload = json.loads(matrix_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, UnicodeError) as error:
        return [f"invalid visual capability matrix: {error}"]

    problems: list[str] = []
    if payload.get("schema_version") != 1:
        problems.append("visual capability matrix schema_version must be 1")
    if payload.get("policy_id") != "PLAN-XPLAT-FIX-001/XPF-WP06":
        problems.append("visual capability matrix policy_id mismatch")
    required_states = [
        "defined", "compiled", "physically_run", "semantically_matched",
        "visual_tolerance_passed", "performance_qualified", "qualified",
    ]
    if payload.get("state_order") != required_states:
        problems.append("visual capability matrix state_order mismatch")
    profiles = payload.get("profiles")
    if not isinstance(profiles, dict) or not profiles:
        return [*problems, "visual capability matrix profiles must be an object"]
    required_profiles = {
        "macos-metal-desktop-v1",
        "windows-d3d12-desktop-v1",
        "ios-metal-contract-canary",
        "android-vulkan-contract-canary",
    }
    if set(profiles) != required_profiles:
        problems.append("visual capability matrix profile set mismatch")

    registry_path = root / "src" / "core" / "VisualContributionRegistry.cpp"
    if not registry_path.is_file():
        return [*problems, "visual contribution registry source is missing"]
    registry_text = registry_path.read_text(encoding="utf-8")
    expected_capabilities = {
        "visual.properties.registry.v1",
        *re.findall(r'\.capability_id\s*=\s*"([^"]+)"', registry_text),
    }
    capabilities = payload.get("capabilities")
    if not isinstance(capabilities, list):
        return [*problems, "visual capability matrix capabilities must be an array"]
    actual_capabilities: set[str] = set()
    descriptor_ids: set[str] = set()
    for capability in capabilities:
        if not isinstance(capability, dict):
            problems.append("visual capability entry must be an object")
            continue
        capability_id = capability.get("capability_id")
        descriptor_id = capability.get("descriptor_id")
        if not isinstance(capability_id, str) or not capability_id:
            problems.append("visual capability entry has no capability_id")
            continue
        if capability_id in actual_capabilities:
            problems.append(f"duplicate visual capability: {capability_id}")
        actual_capabilities.add(capability_id)
        if not isinstance(descriptor_id, str) or not descriptor_id:
            problems.append(f"visual capability {capability_id} has no descriptor_id")
        elif descriptor_id in descriptor_ids:
            problems.append(f"duplicate visual descriptor_id: {descriptor_id}")
        else:
            descriptor_ids.add(descriptor_id)
        states_by_profile = capability.get("profiles")
        if not isinstance(states_by_profile, dict):
            problems.append(f"visual capability {capability_id} has no profiles")
            continue
        if set(states_by_profile) != set(profiles):
            problems.append(
                f"visual capability {capability_id} profile set mismatch"
            )
        for profile_id, state in states_by_profile.items():
            if not isinstance(state, dict):
                problems.append(
                    f"visual capability {capability_id}/{profile_id} state must be an object"
                )
                continue
            values: list[bool] = []
            for state_name in required_states:
                value = state.get(state_name)
                if not isinstance(value, bool):
                    problems.append(
                        f"visual capability {capability_id}/{profile_id} "
                        f"{state_name} must be boolean"
                    )
                    value = False
                values.append(value)
            for index, value in enumerate(values[1:], start=1):
                if value and not all(values[:index]):
                    problems.append(
                        f"visual capability {capability_id}/{profile_id} "
                        f"skips evidence before {required_states[index]}"
                    )
            if values[-1] != all(values[:-1]):
                problems.append(
                    f"visual capability {capability_id}/{profile_id} qualified state is inconsistent"
                )
            evidence = state.get("evidence")
            if not isinstance(evidence, list) or any(
                not isinstance(item, str) or not item for item in evidence
            ):
                problems.append(
                    f"visual capability {capability_id}/{profile_id} evidence must be a string array"
                )
            elif values[0] and not evidence:
                problems.append(
                    f"visual capability {capability_id}/{profile_id} defined state has no evidence"
                )
    if actual_capabilities != expected_capabilities:
        missing = sorted(expected_capabilities - actual_capabilities)
        extra = sorted(actual_capabilities - expected_capabilities)
        problems.append(
            f"visual capability matrix registry mismatch missing={missing} extra={extra}"
        )
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
    visual_boundary_findings = collect_visual_boundary_findings(ROOT)
    problems.extend(studio_authority_problems(ROOT))
    problems.extend(cross_platform_contract_problems(ROOT))
    problems.extend(mobile_contract_canary_problems(ROOT))
    problems.extend(visual_color_contract_problems(ROOT))
    problems.extend(visual_qualification_contract_problems(ROOT))
    problems.extend(windows_bringup_contract_problems(ROOT))
    problems.extend(cross_platform_visual_boundary_problems(ROOT))
    problems.extend(visual_capability_matrix_problems(ROOT))
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
        "visual_boundary_debt": {
            "frozen_signatures": len(visual_boundary_findings),
            "occurrences": sum(
                int(item["occurrences"]) for item in visual_boundary_findings
            ),
            "exception_manifest": str(VISUAL_BOUNDARY_EXCEPTIONS.relative_to(ROOT)),
            "mode": "shrink-only",
        },
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
