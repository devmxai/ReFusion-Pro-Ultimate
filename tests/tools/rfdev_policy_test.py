#!/usr/bin/env python3
"""Negative policy tests for Repo OS checks."""

from __future__ import annotations

import importlib.util
import hashlib
import io
import json
import pathlib
import subprocess
import tempfile
from contextlib import redirect_stdout
from copy import deepcopy


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("refusion_rfdev", ROOT / "tools" / "rfdev.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("unable to load tools/rfdev.py")
RFDEV = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RFDEV)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> None:
    state = RFDEV.current()
    require(
        "PLAN-XPLAT-FIX-001" in state["active_guardrails"],
        "the cross-platform remediation plan is not an active guardrail",
    )
    guardrail = RFDEV.document_path("PLAN-XPLAT-FIX-001")
    require(
        RFDEV.frontmatter(guardrail).get("status") == "active",
        "the active cross-platform guardrail is not active",
    )
    context_output = io.StringIO()
    with redirect_stdout(context_output):
        require(RFDEV.command_context() == 0, "rfdev context failed")
    context = json.loads(context_output.getvalue())
    require(
        any(
            item["path"] == "docs/plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md"
            for item in context["read_set"]
        ),
        "rfdev context did not load the active cross-platform guardrail",
    )

    fixture_directory = (
        ROOT / "tests" / "fixtures" / "media" / "h264-cfr-320x180-gop1"
    )
    fixture_manifest = json.loads(
        (fixture_directory / "fixture.json").read_text(encoding="utf-8")
    )
    fixture_payload = fixture_directory / fixture_manifest["payload"]
    fixture_bytes = fixture_payload.read_bytes()
    require(
        len(fixture_bytes) == fixture_manifest["byte_size"],
        "the bounded H.264 fixture byte size does not match its manifest",
    )
    require(
        hashlib.sha256(fixture_bytes).hexdigest() == fixture_manifest["sha256"],
        "the bounded H.264 fixture digest does not match its manifest",
    )

    with tempfile.TemporaryDirectory(prefix="refusion-policy-") as temporary:
        root = pathlib.Path(temporary)
        studio = root / "apps" / "studio"
        cli = root / "apps" / "cli"
        studio.mkdir(parents=True)
        cli.mkdir(parents=True)
        (studio / "BadBridge.cpp").write_text(
            "ProjectAuthority forbidden_authority;\n", encoding="utf-8"
        )
        (studio / "BadViewport.cpp").write_text(
            "auto gpu = create_platform_gpu_device_service();\n", encoding="utf-8"
        )
        (studio / "CMakeLists.txt").write_text(
            "target_link_libraries(bad PRIVATE ReFusion::Core)\n", encoding="utf-8"
        )
        (cli / "CMakeLists.txt").write_text(
            "target_link_libraries(bad PRIVATE ReFusion::Core)\n", encoding="utf-8"
        )
        problems = RFDEV.studio_authority_problems(root)
        require(len(problems) == 4, f"expected 4 policy failures, got {problems}")

    require(
        not RFDEV.studio_authority_problems(ROOT),
        "the real repository violates the Studio authority boundary",
    )

    with tempfile.TemporaryDirectory(prefix="refusion-cross-platform-") as temporary:
        root = pathlib.Path(temporary)
        core = root / "src" / "core"
        core.mkdir(parents=True)
        (core / "PlatformLeak.cpp").write_text(
            "#ifdef _WIN32\nint platform_meaning = 1;\n#endif\n",
            encoding="utf-8",
        )
        (root / "CMakePresets.json").write_text(
            json.dumps({
                "configurePresets": [{"name": "macos-core"}],
                "workflowPresets": [{"name": "macos-core"}],
            }),
            encoding="utf-8",
        )
        problems = RFDEV.cross_platform_contract_problems(root)
        require(
            any("platform conditional outside an adapter" in value for value in problems),
            f"common platform conditional was accepted: {problems}",
        )
        require(
            any("windows-core" in value for value in problems),
            f"missing Windows lane was accepted: {problems}",
        )

    require(
        not RFDEV.cross_platform_contract_problems(ROOT),
        "the real repository violates the cross-platform build contract",
    )

    with tempfile.TemporaryDirectory(prefix="refusion-product-context-") as temporary:
        root = pathlib.Path(temporary)
        native = root / "src/adapters/skia/SkiaGpuContextsMetal.mm"
        native.parent.mkdir(parents=True)
        native.write_text(
            '#include "include/gpu/graphite/Context.h"\n'
            "auto bad = skgpu::graphite::ContextFactory::MakeMetal();\n",
            encoding="utf-8",
        )
        problems = RFDEV.cross_platform_contract_problems(root)
        require(
            any("unqualified second product render context" in value for value in problems),
            f"a second product render context was accepted: {problems}",
        )

    with tempfile.TemporaryDirectory(prefix="refusion-mobile-canary-") as temporary:
        root = pathlib.Path(temporary)
        ios = root / "src/platform/apple/metal_ios/IosMetalContractCanary.mm"
        ios.parent.mkdir(parents=True)
        ios.write_text("#import <AppKit/AppKit.h>\n", encoding="utf-8")
        problems = RFDEV.mobile_contract_canary_problems(root)
        require(
            any("imports desktop AppKit" in value for value in problems),
            f"desktop AppKit leaked into the iOS canary: {problems}",
        )

    require(
        not RFDEV.mobile_contract_canary_problems(ROOT),
        "the real repository violates the mobile contract-canary boundary",
    )

    with tempfile.TemporaryDirectory(prefix="refusion-visual-boundary-") as temporary:
        root = pathlib.Path(temporary)
        native = root / "src" / "adapters" / "skia" / "BadMetal.mm"
        common = root / "src" / "adapters" / "skia" / "BadCommon.cpp"
        common_cmake = root / "src" / "adapters" / "skia" / "CMakeLists.txt"
        studio = root / "apps" / "studio" / "BadEffects.qml"
        native.parent.mkdir(parents=True)
        studio.parent.mkdir(parents=True)
        native.write_text(
            "void bad(core::GlowEffect value) {}\n", encoding="utf-8"
        )
        common.write_text(
            "#if defined(__APPLE__)\n"
            "#include \"include/ports/SkFontMgr_mac_ct.h\"\n"
            "#endif\n",
            encoding="utf-8",
        )
        common_cmake.write_text(
            "target_link_libraries(refusion_skia_adapter PRIVATE "
            "ReFusion::PlatformMedia)\n",
            encoding="utf-8",
        )
        studio.write_text(
            'Button { onClicked: studioBridge.addSelectedEffect("new_fx") }\n'
            'Item { visible: modelData.kind === "new_fx" }\n',
            encoding="utf-8",
        )
        findings = RFDEV.collect_visual_boundary_findings(root)
        rules = {str(item["rule"]) for item in findings}
        require(
            {
                "native-project-semantics",
                "common-platform-branch",
                "common-native-include",
                "common-native-link",
                "studio-effect-vocabulary",
            }.issubset(rules),
            f"visual boundary negative fixtures were not detected: {findings}",
        )
        problems = RFDEV.cross_platform_visual_boundary_problems(root)
        require(
            sum("unapproved visual-boundary violation" in value for value in problems)
            >= 5,
            f"visual boundary accepted unapproved semantics: {problems}",
        )

    require(
        not RFDEV.cross_platform_visual_boundary_problems(ROOT),
        "the real visual-boundary ratchet does not match its frozen baseline",
    )
    require(
        not RFDEV.visual_capability_matrix_problems(ROOT),
        "the real visual capability matrix is inconsistent",
    )
    require(
        not RFDEV.visual_qualification_contract_problems(ROOT),
        "the real visual qualification contract is inconsistent",
    )

    capability_matrix = json.loads(
        RFDEV.VISUAL_CAPABILITY_MATRIX.read_text(encoding="utf-8")
    )
    with tempfile.TemporaryDirectory(prefix="refusion-capability-matrix-") as temporary:
        bad_matrix = pathlib.Path(temporary) / "matrix.json"
        skipped_evidence = deepcopy(capability_matrix)
        state = skipped_evidence["capabilities"][0]["profiles"][
            "android-vulkan-contract-canary"
        ]
        state["physically_run"] = True
        bad_matrix.write_text(json.dumps(skipped_evidence), encoding="utf-8")
        problems = RFDEV.visual_capability_matrix_problems(ROOT, bad_matrix)
        require(
            any("skips evidence" in value for value in problems),
            f"capability matrix accepted an evidence jump: {problems}",
        )

    manifest = json.loads(RFDEV.VISUAL_BOUNDARY_EXCEPTIONS.read_text(encoding="utf-8"))
    with tempfile.TemporaryDirectory(prefix="refusion-visual-ratchet-") as temporary:
        bad_manifest = pathlib.Path(temporary) / "exceptions.json"
        changed_baseline = deepcopy(manifest)
        changed_baseline["frozen_baseline"][0]["symbol"] = "replacement-symbol"
        bad_manifest.write_text(json.dumps(changed_baseline), encoding="utf-8")
        problems = RFDEV.cross_platform_visual_boundary_problems(
            ROOT, bad_manifest, True
        )
        require(
            any("frozen baseline changed" in value for value in problems),
            f"visual boundary accepted a changed frozen signature: {problems}",
        )

        new_allowance = deepcopy(manifest)
        new_allowance["active_allowances"]["xpf-new-debt"] = 1
        bad_manifest.write_text(json.dumps(new_allowance), encoding="utf-8")
        problems = RFDEV.cross_platform_visual_boundary_problems(
            ROOT, bad_manifest, True
        )
        require(
            any("new visual-boundary allowance is forbidden" in value for value in problems),
            f"visual boundary accepted a new allowance: {problems}",
        )

        grown_allowance = deepcopy(manifest)
        grown_allowance["active_allowances"]["xpf-visual-debt-001"] = 2
        bad_manifest.write_text(json.dumps(grown_allowance), encoding="utf-8")
        problems = RFDEV.cross_platform_visual_boundary_problems(
            ROOT, bad_manifest, True
        )
        require(
            any("allowance grew" in value for value in problems),
            f"visual boundary accepted a grown allowance: {problems}",
        )

    with tempfile.TemporaryDirectory(prefix="refusion-release-gate-") as temporary:
        completed = subprocess.run(
            [
                "cmake", "-S", str(ROOT), "-B", temporary,
                "-DREFUSION_BUILD_STUDIO=ON",
                "-DREFUSION_RELEASE_BUILD=ON",
                "-DBUILD_TESTING=OFF",
            ],
            check=False,
            text=True,
            capture_output=True,
        )
        output = completed.stdout + completed.stderr
        require(completed.returncode != 0, "release configure accepted missing Qt authority")
        require(
            "REFUSION_QT_COMMERCIAL_SDK_ROOT" in output,
            f"release configure failed for the wrong reason: {output}",
        )

    with tempfile.TemporaryDirectory(prefix="refusion-skia-path-gate-") as temporary:
        outside = pathlib.Path(temporary) / "outside"
        completed = subprocess.run(
            [
                "cmake", "-S", str(ROOT), "-B", str(pathlib.Path(temporary) / "build"),
                "-DREFUSION_ENABLE_SKIA=ON",
                f"-DREFUSION_SKIA_SOURCE_DIR={outside / 'src'}",
                f"-DREFUSION_SKIA_BUILD_DIR={outside / 'build-artifact'}",
                "-DBUILD_TESTING=OFF",
            ],
            check=False,
            text=True,
            capture_output=True,
        )
        output = completed.stdout + completed.stderr
        require(completed.returncode != 0, "CMake accepted Skia outside ReFusion")
        require(
            "or resolve from the verified ReFusion development machine cache" in output,
            f"Skia path gate failed for the wrong reason: {output}",
        )

    dependency_record = ROOT / "out" / "deps-src" / "skia-dependencies.lock.json"
    if dependency_record.is_file():
        bootstrap_spec = importlib.util.spec_from_file_location(
            "refusion_bootstrap", ROOT / "tools" / "bootstrap.py"
        )
        if bootstrap_spec is None or bootstrap_spec.loader is None:
            raise RuntimeError("unable to load tools/bootstrap.py")
        bootstrap = importlib.util.module_from_spec(bootstrap_spec)
        bootstrap_spec.loader.exec_module(bootstrap)
        with tempfile.TemporaryDirectory(prefix="refusion-skia-tamper-") as temporary:
            bad_record = pathlib.Path(temporary) / "skia-dependencies.lock.json"
            value = json.loads(dependency_record.read_text(encoding="utf-8"))
            value["deps_sha256"] = "0" * 64
            bad_record.write_text(json.dumps(value), encoding="utf-8")
            original = bootstrap.SKIA_DEPENDENCY_RECORD
            bootstrap.SKIA_DEPENDENCY_RECORD = bad_record
            rejected = False
            try:
                with redirect_stdout(io.StringIO()):
                    bootstrap.verify_skia_materialization(bootstrap.SOURCE_CACHE)
            except RuntimeError as error:
                rejected = "DEPS changed" in str(error)
            finally:
                bootstrap.SKIA_DEPENDENCY_RECORD = original
            require(rejected, "Skia materialization accepted a tampered DEPS digest")


if __name__ == "__main__":
    main()
