#!/usr/bin/env python3
"""Pinned, explicit dependency bootstrap for ReFusion foundation."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import platform
import shutil
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
LOCK = ROOT / "deps" / "manifest.lock.json"
SKIA_PROFILES = ROOT / "deps" / "profiles" / "skia" / "profiles.json"
TOOLCHAIN_ROOT = ROOT / "deps" / "toolchains"
TRANSITIVE_LOCK_ROOT = ROOT / "deps" / "locks"
SOURCE_CACHE = ROOT / "out" / "deps-src"
SKIA_BUILD_ROOT = ROOT / "out" / "deps-build" / "skia"
SKIA_DEPENDENCY_RECORD = SOURCE_CACHE / "skia-dependencies.lock.json"


def run(
    command: list[str],
    cwd: pathlib.Path | None = None,
    env: dict[str, str] | None = None,
) -> str:
    completed = subprocess.run(
        command, cwd=cwd, env=env, check=False, text=True, capture_output=True
    )
    if completed.returncode:
        detail = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"command failed ({completed.returncode}): {' '.join(command)}\n{detail}")
    return completed.stdout.strip()


def manifest() -> dict:
    return json.loads(LOCK.read_text(encoding="utf-8"))


def first_line(command: list[str]) -> str:
    try:
        return run(command).splitlines()[0]
    except (RuntimeError, FileNotFoundError) as error:
        return f"MISSING ({error})"


def doctor() -> int:
    checks = {
        "git": first_line(["git", "--version"]),
        "cmake": first_line(["cmake", "--version"]),
        "ninja": first_line(["ninja", "--version"]),
        "python": sys.version.splitlines()[0],
        "qt6": first_line(["pkg-config", "--modversion", "Qt6Core"]),
    }
    if sys.platform == "darwin" and platform.machine() == "arm64":
        checks.update({
            "xcode": first_line(["xcodebuild", "-version"]),
            "macos_sdk": first_line(["xcrun", "--sdk", "macosx", "--show-sdk-version"]),
            "apple_clang": first_line(["/usr/bin/clang", "--version"]),
        })
        toolchain_path = TOOLCHAIN_ROOT / "macos-arm64-development.json"
    elif os.name == "nt" and platform.machine().lower() in {"amd64", "x86_64"}:
        toolchain_path = TOOLCHAIN_ROOT / "windows-x64-development.json"
    else:
        toolchain_path = None

    mismatches: list[dict[str, str]] = []
    toolchain = None
    if toolchain_path is not None and toolchain_path.is_file():
        toolchain = json.loads(toolchain_path.read_text(encoding="utf-8"))
        if toolchain.get("status") == "qualified-local":
            for name, expected in toolchain.get("versions", {}).items():
                actual = checks.get(name, "MISSING")
                if name == "python":
                    actual = platform.python_version()
                if actual != expected:
                    mismatches.append({
                        "component": name,
                        "expected": expected,
                        "actual": actual,
                    })
    print(json.dumps({
        "lock": str(LOCK),
        "toolchain_lock": str(toolchain_path) if toolchain_path else None,
        "toolchain_status": toolchain.get("status") if toolchain else "unqualified-host",
        "checks": checks,
        "mismatches": mismatches,
    }, indent=2))
    missing = [name for name, value in checks.items() if value.startswith("MISSING")]
    required_missing = any(name in {"git", "cmake", "ninja"} for name in missing)
    return 1 if required_missing or mismatches else 0


def component(name: str) -> dict:
    components = manifest()["components"]
    if name not in components:
        raise RuntimeError(f"unknown component: {name}")
    return components[name]


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_json_atomic(path: pathlib.Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def host_key() -> str:
    if sys.platform == "darwin" and platform.machine() == "arm64":
        return "macos-arm64"
    if os.name == "nt" and platform.machine().lower() in {"amd64", "x86_64"}:
        return "windows-x64"
    return f"{sys.platform}-{platform.machine().lower()}"


def tracked_transitive_lock_path() -> pathlib.Path:
    return TRANSITIVE_LOCK_ROOT / f"skia-transitive-{host_key()}.lock.json"


def transitive_lock_projection(record: dict) -> dict:
    return {
        "schema_version": 1,
        "host": host_key(),
        "skia_origin": record["skia_origin"],
        "skia_revision": record["skia_revision"],
        "depot_tools_origin": record["depot_tools_origin"],
        "depot_tools_revision": record["depot_tools_revision"],
        "deps_sha256": record["deps_sha256"],
        "git_dependencies": record["git_dependencies"],
    }


def controlled_destination(name: str, cache: pathlib.Path) -> pathlib.Path:
    resolved_cache = cache.resolve()
    if resolved_cache != SOURCE_CACHE.resolve():
        raise RuntimeError(
            f"dependency sources must remain inside ReFusion: {SOURCE_CACHE}"
        )
    destination = resolved_cache / name
    if destination.parent != resolved_cache or destination.name != name:
        raise RuntimeError(f"refusing uncontrolled dependency path: {destination}")
    if destination.is_symlink():
        raise RuntimeError(f"refusing dependency symlink: {destination}")
    return destination


def verify_git(name: str, source: pathlib.Path) -> int:
    spec = component(name)
    if spec.get("kind") != "git":
        raise RuntimeError(f"{name} is not a git component")
    if not source.is_dir():
        raise RuntimeError(f"source does not exist: {source}")
    head = run(["git", "rev-parse", "HEAD"], cwd=source)
    origin = run(["git", "remote", "get-url", "origin"], cwd=source)
    expected_head = spec["revision"]
    expected_origin = spec["official_origin"]
    dirty = run(["git", "status", "--porcelain"], cwd=source)
    ok = (
        head == expected_head
        and origin.rstrip("/") == expected_origin.rstrip("/")
        and not dirty
    )
    print(json.dumps({
        "component": name,
        "source": str(source),
        "origin": origin,
        "expected_origin": expected_origin,
        "head": head,
        "expected_head": expected_head,
        "clean": not dirty,
        "verified": ok,
    }, indent=2))
    return 0 if ok else 2


def sync_git(name: str, cache: pathlib.Path, fresh: bool) -> int:
    spec = component(name)
    if spec.get("kind") != "git":
        raise RuntimeError(f"{name} cannot be synced automatically")
    cache = cache.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    destination = controlled_destination(name, cache)
    if fresh and destination.exists():
        shutil.rmtree(destination)
    if not destination.exists():
        run(["git", "clone", "--filter=blob:none", "--no-checkout", "--no-tags",
             spec["official_origin"], str(destination)])
    origin = run(["git", "remote", "get-url", "origin"], cwd=destination)
    if origin.rstrip("/") != spec["official_origin"].rstrip("/"):
        raise RuntimeError(f"refusing unexpected origin for {name}: {origin}")
    run(["git", "fetch", "--depth", "1", "origin", spec["revision"]], cwd=destination)
    run(["git", "checkout", "--detach", spec["revision"]], cwd=destination)
    return verify_git(name, destination)


def git_dependency_inventory(root: pathlib.Path) -> list[dict[str, str]]:
    inventory: list[dict[str, str]] = []
    externals = root / "third_party" / "externals"
    if not externals.is_dir():
        return inventory
    for dependency in sorted(path for path in externals.iterdir() if path.is_dir()):
        try:
            head = run(["git", "rev-parse", "HEAD"], cwd=dependency)
            origin = run(["git", "remote", "get-url", "origin"], cwd=dependency)
            dirty = run(["git", "status", "--porcelain"], cwd=dependency)
        except RuntimeError as error:
            raise RuntimeError(
                f"Skia dependency is not a verifiable Git checkout: {dependency}"
            ) from error
        if dirty:
            raise RuntimeError(f"Skia dependency worktree is modified: {dependency}")
        inventory.append({
            "path": str(dependency.relative_to(root)),
            "origin": origin,
            "revision": head,
            "clean": True,
        })
    return inventory


def verify_skia_materialization(cache: pathlib.Path, emit: bool = True) -> dict:
    cache = cache.resolve()
    skia = controlled_destination("skia", cache)
    depot_tools = controlled_destination("depot_tools", cache)
    if verify_git("skia", skia) != 0 or verify_git("depot_tools", depot_tools) != 0:
        raise RuntimeError("Skia roots do not match the foundation lock")
    if not SKIA_DEPENDENCY_RECORD.is_file():
        raise RuntimeError("Skia dependency record is missing; run hydrate-skia")
    record = json.loads(SKIA_DEPENDENCY_RECORD.read_text(encoding="utf-8"))
    if record.get("schema_version") != 2:
        raise RuntimeError("Skia dependency record schema is stale; run hydrate-skia")
    if record.get("skia_revision") != component("skia")["revision"]:
        raise RuntimeError("Skia dependency record has an unexpected root revision")
    if record.get("depot_tools_revision") != component("depot_tools")["revision"]:
        raise RuntimeError("Skia dependency record has an unexpected depot_tools revision")
    if record.get("deps_sha256") != sha256_file(skia / "DEPS"):
        raise RuntimeError("Skia DEPS changed after hydration")

    actual_inventory = git_dependency_inventory(skia)
    expected_inventory = record.get("git_dependencies", [])
    if actual_inventory != expected_inventory:
        raise RuntimeError("Skia dependency origin/revision/clean inventory drifted")

    tracked_lock = tracked_transitive_lock_path()
    if not tracked_lock.is_file():
        raise RuntimeError(
            f"tracked Skia transitive lock is missing for {host_key()}; "
            "run lock-skia-materialization and review it"
        )
    tracked_value = json.loads(tracked_lock.read_text(encoding="utf-8"))
    if tracked_value != transitive_lock_projection(record):
        raise RuntimeError("Skia materialization differs from the tracked transitive lock")

    expected_tools = record.get("tools", {})
    for name, tool in expected_tools.items():
        path = skia / tool["path"]
        if not path.is_file() or sha256_file(path) != tool["sha256"]:
            raise RuntimeError(f"Skia hydrated tool changed: {name}")

    result = {
        "verified": True,
        "dependency_count": len(actual_inventory),
        "record": str(SKIA_DEPENDENCY_RECORD),
        "record_sha256": sha256_file(SKIA_DEPENDENCY_RECORD),
        "tracked_lock": str(tracked_lock),
        "tracked_lock_sha256": sha256_file(tracked_lock),
    }
    if emit:
        print(json.dumps(result, indent=2))
    return result


def lock_skia_materialization(cache: pathlib.Path) -> int:
    resolved_cache = cache.resolve()
    skia = controlled_destination("skia", resolved_cache)
    depot_tools = controlled_destination("depot_tools", resolved_cache)
    if verify_git("skia", skia) != 0 or verify_git("depot_tools", depot_tools) != 0:
        raise RuntimeError("cannot lock unverified Skia roots")
    record_path = resolved_cache / "skia-dependencies.lock.json"
    if not record_path.is_file():
        raise RuntimeError("hydrate Skia before generating its tracked transitive lock")
    record = json.loads(record_path.read_text(encoding="utf-8"))
    if record.get("schema_version") != 2:
        raise RuntimeError("hydrate Skia with the current schema before locking it")
    if record.get("deps_sha256") != sha256_file(skia / "DEPS"):
        raise RuntimeError("cannot lock a Skia record with a mismatched DEPS digest")
    if record.get("git_dependencies") != git_dependency_inventory(skia):
        raise RuntimeError("cannot lock a drifted Skia dependency inventory")
    path = tracked_transitive_lock_path()
    write_json_atomic(path, transitive_lock_projection(record))
    print(json.dumps({
        "locked": True,
        "host": host_key(),
        "path": str(path.relative_to(ROOT)),
        "sha256": sha256_file(path),
    }, indent=2))
    return 0


def hydrate_skia(cache: pathlib.Path) -> int:
    cache = cache.resolve()
    skia = controlled_destination("skia", cache)
    depot_tools = controlled_destination("depot_tools", cache)
    if verify_git("skia", skia) != 0 or verify_git("depot_tools", depot_tools) != 0:
        raise RuntimeError("refusing to hydrate unverified Skia sources")

    environment = os.environ.copy()
    environment["PATH"] = str(depot_tools) + os.pathsep + environment.get("PATH", "")
    run([sys.executable, "tools/git-sync-deps"], cwd=skia, env=environment)
    run([sys.executable, "bin/fetch-ninja"], cwd=skia, env=environment)

    record = {
        "schema_version": 2,
        "skia_origin": component("skia")["official_origin"],
        "skia_revision": component("skia")["revision"],
        "depot_tools_origin": component("depot_tools")["official_origin"],
        "depot_tools_revision": component("depot_tools")["revision"],
        "deps_sha256": sha256_file(skia / "DEPS"),
        "git_dependencies": git_dependency_inventory(skia),
        "tools": {
            name: {
                "path": str(path.relative_to(skia)),
                "sha256": sha256_file(path),
            }
            for name, path in {
                "gn": skia / "bin" / "gn",
                "ninja": skia / "third_party" / "ninja" /
                ("ninja.exe" if os.name == "nt" else "ninja"),
            }.items()
            if path.is_file()
        },
    }
    record_path = cache / "skia-dependencies.lock.json"
    write_json_atomic(record_path, record)
    print(json.dumps({
        "hydrated": True,
        "source": str(skia),
        "dependency_count": len(record["git_dependencies"]),
        "record": str(record_path),
    }, indent=2))
    return 0


def skia_profiles() -> dict:
    return json.loads(SKIA_PROFILES.read_text(encoding="utf-8"))["profiles"]


def build_skia(profile_name: str, cache: pathlib.Path, build_root: pathlib.Path) -> int:
    profiles = skia_profiles()
    if profile_name not in profiles:
        raise RuntimeError(f"unknown Skia build profile: {profile_name}")
    profile = profiles[profile_name]
    cache = cache.resolve()
    skia = controlled_destination("skia", cache)
    if verify_git("skia", skia) != 0:
        raise RuntimeError("refusing to build unverified Skia sources")
    materialization = verify_skia_materialization(cache, emit=False)

    gn = skia / "bin" / "gn"
    ninja = (skia / "third_party" / "ninja" /
             ("ninja.exe" if os.name == "nt" else "ninja"))
    if not gn.is_file() or not ninja.is_file():
        raise RuntimeError("Skia GN/Ninja tools are missing; run hydrate-skia first")

    args_path = SKIA_PROFILES.parent / profile["gn_args"]
    args = args_path.read_text(encoding="utf-8")
    if build_root.resolve() != SKIA_BUILD_ROOT.resolve():
        raise RuntimeError(f"Skia builds must remain inside ReFusion: {SKIA_BUILD_ROOT}")
    output = build_root.resolve() / profile_name
    output.mkdir(parents=True, exist_ok=True)
    run([str(gn), "gen", str(output), f"--args={args}"], cwd=skia)
    run([str(ninja), "-C", str(output), *profile["targets"]], cwd=skia)

    bundle = output / profile["bundle_artifact"]
    if bundle.exists():
        bundle.unlink()
    archive_pattern = "*.lib" if os.name == "nt" else "*.a"
    component_archives = sorted(output.glob(archive_pattern))
    has_primary = any(
        path.name in {"libskia.a", "skia.lib"} for path in component_archives
    )
    if not component_archives or not has_primary:
        raise RuntimeError("Skia link closure does not contain the primary library")
    if os.name == "nt":
        librarian = shutil.which("lib.exe") or shutil.which("lib")
        if librarian is None:
            raise RuntimeError("MSVC librarian was not found in the active toolchain")
        run([
            librarian,
            "/NOLOGO",
            "/BREPRO",
            f"/OUT:{bundle}",
            *[str(path) for path in component_archives],
        ])
    elif sys.platform == "darwin":
        deterministic_environment = os.environ.copy()
        deterministic_environment["ZERO_AR_DATE"] = "1"
        run([
            "/usr/bin/libtool",
            "-static",
            "-o",
            str(bundle),
            *[str(path) for path in component_archives],
        ], env=deterministic_environment)
    else:
        raise RuntimeError("no admitted Skia archive bundler for this host")

    artifact = {
        "path": str(bundle.relative_to(ROOT)),
        "size": bundle.stat().st_size,
        "sha256": sha256_file(bundle),
    }
    archive_records = [
        {
            "path": str(path.relative_to(ROOT)),
            "size": path.stat().st_size,
            "sha256": sha256_file(path),
        }
        for path in component_archives
    ]

    record = {
        "schema_version": 2,
        "profile": profile_name,
        "source_origin": component("skia")["official_origin"],
        "source_revision": component("skia")["revision"],
        "gn_args": str(args_path.relative_to(ROOT)),
        "gn_args_sha256": sha256_file(args_path),
        "dependency_record": str(SKIA_DEPENDENCY_RECORD.relative_to(ROOT)),
        "dependency_record_sha256": materialization["record_sha256"],
        "tracked_dependency_lock": str(
            pathlib.Path(materialization["tracked_lock"]).relative_to(ROOT)
        ),
        "tracked_dependency_lock_sha256": materialization["tracked_lock_sha256"],
        "dependency_count": materialization["dependency_count"],
        "host": {
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
        },
        "toolchain": {
            "python": sys.version.splitlines()[0],
            "cmake": first_line(["cmake", "--version"]),
            "gn_sha256": sha256_file(gn),
            "ninja_sha256": sha256_file(ninja),
            "cc": first_line(["/usr/bin/clang", "--version"])
            if sys.platform == "darwin" else first_line(["cl"]),
        },
        "targets": profile["targets"],
        "component_archives": archive_records,
        "artifact": artifact,
    }
    record_path = output / "refusion-build.json"
    write_json_atomic(record_path, record)
    print(json.dumps(record, indent=2))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("doctor")
    verify = sub.add_parser("verify")
    verify.add_argument("component")
    verify.add_argument("--source", required=True, type=pathlib.Path)
    sync = sub.add_parser("sync")
    sync.add_argument("component")
    sync.add_argument("--fresh", action="store_true")
    sub.add_parser("hydrate-skia")
    sub.add_parser("verify-skia-materialization")
    sub.add_parser("lock-skia-materialization")
    build = sub.add_parser("build-skia")
    build.add_argument("--profile", required=True, choices=tuple(skia_profiles()))
    args = parser.parse_args()
    try:
        if args.command == "doctor":
            return doctor()
        if args.command == "verify":
            return verify_git(args.component, args.source.resolve())
        if args.command == "sync":
            return sync_git(args.component, SOURCE_CACHE, args.fresh)
        if args.command == "hydrate-skia":
            return hydrate_skia(SOURCE_CACHE)
        if args.command == "verify-skia-materialization":
            verify_skia_materialization(SOURCE_CACHE)
            return 0
        if args.command == "lock-skia-materialization":
            return lock_skia_materialization(SOURCE_CACHE)
        return build_skia(args.profile, SOURCE_CACHE, SKIA_BUILD_ROOT)
    except (RuntimeError, OSError, json.JSONDecodeError) as error:
        print(f"bootstrap error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
