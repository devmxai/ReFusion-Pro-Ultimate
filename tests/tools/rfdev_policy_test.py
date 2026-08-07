#!/usr/bin/env python3
"""Negative policy tests for Repo OS checks."""

from __future__ import annotations

import importlib.util
import io
import json
import pathlib
import subprocess
import tempfile
from contextlib import redirect_stdout


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
    with tempfile.TemporaryDirectory(prefix="refusion-policy-") as temporary:
        root = pathlib.Path(temporary)
        studio = root / "apps" / "studio"
        cli = root / "apps" / "cli"
        studio.mkdir(parents=True)
        cli.mkdir(parents=True)
        (studio / "BadBridge.cpp").write_text(
            "ProjectAuthority forbidden_authority;\n", encoding="utf-8"
        )
        (studio / "CMakeLists.txt").write_text(
            "target_link_libraries(bad PRIVATE ReFusion::Core)\n", encoding="utf-8"
        )
        (cli / "CMakeLists.txt").write_text(
            "target_link_libraries(bad PRIVATE ReFusion::Core)\n", encoding="utf-8"
        )
        problems = RFDEV.studio_authority_problems(root)
        require(len(problems) == 3, f"expected 3 policy failures, got {problems}")

    require(
        not RFDEV.studio_authority_problems(ROOT),
        "the real repository violates the Studio authority boundary",
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
            "must remain inside this ReFusion checkout" in output,
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
