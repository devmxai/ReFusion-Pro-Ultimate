#!/usr/bin/env python3
"""Pinned, explicit dependency bootstrap for ReFusion foundation."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import platform
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import urllib.request
import uuid
import zipfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
LOCK = ROOT / "deps" / "manifest.lock.json"
SKIA_PROFILES = ROOT / "deps" / "profiles" / "skia" / "profiles.json"
FFMPEG_PROFILES = ROOT / "deps" / "profiles" / "ffmpeg" / "profiles.json"
TOOLCHAIN_ROOT = ROOT / "deps" / "toolchains"
TRANSITIVE_LOCK_ROOT = ROOT / "deps" / "locks"
SOURCE_CACHE = ROOT / "out" / "deps-src"
SKIA_BUILD_ROOT = ROOT / "out" / "deps-build" / "skia"
FFMPEG_BUILD_ROOT = ROOT / "out" / "deps-build" / "ffmpeg"
SKIA_DEPENDENCY_RECORD = SOURCE_CACHE / "skia-dependencies.lock.json"
MACHINE_CACHE_SCHEMA_VERSION = 1


def default_machine_cache_root() -> pathlib.Path:
    override = os.environ.get("REFUSION_MACHINE_CACHE_ROOT")
    if override:
        return pathlib.Path(override).expanduser().resolve()
    if os.name == "nt":
        # Skia/Dawn contains paths near MAX_PATH. Keep the user-owned default
        # short so a cache publication never truncates a valid checkout.
        return (pathlib.Path.home() / ".rfx" / "dc1").resolve()
    if sys.platform == "darwin":
        return (
            pathlib.Path.home()
            / "Library"
            / "Caches"
            / "ReFusion"
            / "dependency-cache"
            / "v1"
        ).resolve()
    xdg_cache = os.environ.get("XDG_CACHE_HOME")
    base = pathlib.Path(xdg_cache) if xdg_cache else pathlib.Path.home() / ".cache"
    return (base / "refusion" / "dependency-cache" / "v1").resolve()


def machine_cache_index_path(root: pathlib.Path) -> pathlib.Path:
    return root / "index.json"


def empty_machine_cache_index() -> dict:
    return {
        "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
        "skia_profiles": {},
        "qt_sdks": {},
    }


def read_machine_cache_index(root: pathlib.Path) -> dict:
    path = machine_cache_index_path(root)
    if not path.is_file():
        return empty_machine_cache_index()
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise RuntimeError(f"malformed ReFusion machine-cache index: {path}")
    if value.get("schema_version") != MACHINE_CACHE_SCHEMA_VERSION:
        raise RuntimeError(f"unsupported ReFusion machine-cache index: {path}")
    if (
        not isinstance(value.get("skia_profiles"), dict)
        or not isinstance(value.get("qt_sdks"), dict)
    ):
        raise RuntimeError(f"malformed ReFusion machine-cache index: {path}")
    return value


def canonical_sha256(value: dict) -> str:
    payload = json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def is_cache_key(value: str) -> bool:
    return len(value) == 32 and all(
        character in "0123456789abcdef" for character in value
    )


def is_sha256(value: object) -> bool:
    return isinstance(value, str) and len(value) == 64 and all(
        character in "0123456789abcdef" for character in value
    )


def machine_source_cache_is_admitted(cache: pathlib.Path, root: pathlib.Path | None = None) -> bool:
    cache = cache.resolve()
    root = (root or default_machine_cache_root()).resolve()
    try:
        relative = cache.relative_to(root)
    except ValueError:
        return False
    parts = relative.parts
    return (
        len(parts) == 3
        and parts[0] == "s"
        and is_cache_key(parts[1])
        and parts[2] == "d"
    )


def checkout_source_cache_is_compatible(cache: pathlib.Path) -> bool:
    cache = cache.resolve()
    if cache.name != "deps-src" or cache.parent.name != "out":
        return False
    checkout = cache.parent.parent
    candidate_lock = checkout / "deps" / "manifest.lock.json"
    return (
        (checkout / ".git").exists()
        and candidate_lock.is_file()
        and sha256_file(candidate_lock) == sha256_file(LOCK)
    )


def filesystem_io_path(path: pathlib.Path) -> str:
    absolute = str(path.resolve())
    if os.name != "nt" or absolute.startswith("\\\\?\\"):
        return absolute
    if absolute.startswith("\\\\"):
        return "\\\\?\\UNC\\" + absolute[2:]
    return "\\\\?\\" + absolute


def copy_tree_atomic(
    source: pathlib.Path,
    destination: pathlib.Path,
    staging_root: pathlib.Path | None = None,
) -> None:
    if destination.exists():
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    staging_root = staging_root or destination.parent
    staging_root.mkdir(parents=True, exist_ok=True)
    temporary = staging_root / uuid.uuid4().hex[:8]
    try:
        shutil.copytree(
            filesystem_io_path(source),
            filesystem_io_path(temporary),
            symlinks=True,
            copy_function=shutil.copy2,
        )
        temporary.replace(destination)
    finally:
        if temporary.exists():
            remove_tree(temporary)


def remove_tree(path: pathlib.Path) -> None:
    def make_writable_and_retry(function, failed_path, _error) -> None:
        os.chmod(failed_path, stat.S_IWRITE | stat.S_IREAD)
        function(failed_path)

    shutil.rmtree(filesystem_io_path(path), onerror=make_writable_and_retry)


def copy_file_atomic(source: pathlib.Path, destination: pathlib.Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(f".{destination.name}.tmp-{uuid.uuid4().hex}")
    try:
        shutil.copy2(filesystem_io_path(source), filesystem_io_path(temporary))
        temporary.replace(destination)
    finally:
        if temporary.exists():
            temporary.unlink()


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
        raise RuntimeError(
            f"command failed ({completed.returncode}): {' '.join(command)}\n{detail}"
        )
    return completed.stdout.strip()


def git_command(*arguments: str) -> list[str]:
    command = ["git"]
    if os.name == "nt":
        command.extend(("-c", "core.longpaths=true"))
    command.extend(arguments)
    return command


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


def portable_text_sha256_variants(path: pathlib.Path) -> set[str]:
    payload = path.read_bytes()
    try:
        text = payload.decode("utf-8")
    except UnicodeDecodeError as error:
        raise RuntimeError(f"expected UTF-8 dependency metadata: {path}") from error
    lf = text.replace("\r\n", "\n").replace("\r", "\n").encode("utf-8")
    crlf = lf.decode("utf-8").replace("\n", "\r\n").encode("utf-8")
    return {
        hashlib.sha256(payload).hexdigest(),
        hashlib.sha256(lf).hexdigest(),
        hashlib.sha256(crlf).hexdigest(),
    }


def recorded_text_sha256_matches(path: pathlib.Path, recorded: object) -> bool:
    return isinstance(recorded, str) and recorded in portable_text_sha256_variants(path)


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


def controlled_destination(
    name: str,
    cache: pathlib.Path,
    *,
    allow_machine_cache: bool = False,
    allow_compatible_checkout: bool = False,
) -> pathlib.Path:
    resolved_cache = cache.resolve()
    admitted_local = resolved_cache == SOURCE_CACHE.resolve()
    admitted_machine = allow_machine_cache and machine_source_cache_is_admitted(
        resolved_cache
    )
    admitted_checkout = (
        allow_compatible_checkout
        and checkout_source_cache_is_compatible(resolved_cache)
    )
    if not admitted_local and not admitted_machine and not admitted_checkout:
        raise RuntimeError(
            "dependency sources must remain in this ReFusion checkout or a "
            "verified ReFusion machine-cache entry"
        )
    destination = resolved_cache / name
    if destination.parent != resolved_cache or destination.name != name:
        raise RuntimeError(f"refusing uncontrolled dependency path: {destination}")
    if destination.is_symlink():
        raise RuntimeError(f"refusing dependency symlink: {destination}")
    return destination


def skia_dependency_record_path(cache: pathlib.Path) -> pathlib.Path:
    return cache.resolve() / "skia-dependencies.lock.json"


def verify_git(name: str, source: pathlib.Path, *, emit: bool = True) -> int:
    spec = component(name)
    if spec.get("kind") != "git":
        raise RuntimeError(f"{name} is not a git component")
    if not source.is_dir():
        raise RuntimeError(f"source does not exist: {source}")
    head = run(git_command("rev-parse", "HEAD"), cwd=source)
    origin = run(git_command("remote", "get-url", "origin"), cwd=source)
    expected_head = spec["revision"]
    expected_origin = spec["official_origin"]
    dirty = run(git_command("status", "--porcelain"), cwd=source)
    tag = spec.get("tag")
    tag_object = None
    peeled_tag = None
    tag_ok = True
    if tag:
        tag_object = run(git_command("rev-parse", f"refs/tags/{tag}"), cwd=source)
        peeled_tag = run(
            git_command("rev-parse", f"refs/tags/{tag}^{{}}"), cwd=source
        )
        tag_ok = (
            tag_object == spec.get("tag_object")
            and peeled_tag == expected_head
        )
    ok = (
        head == expected_head
        and origin.rstrip("/") == expected_origin.rstrip("/")
        and not dirty
        and tag_ok
    )
    if emit:
        print(json.dumps({
            "component": name,
            "source": str(source),
            "origin": origin,
            "expected_origin": expected_origin,
            "head": head,
            "expected_head": expected_head,
            "clean": not dirty,
            "tag": tag,
            "tag_object": tag_object,
            "peeled_tag": peeled_tag,
            "tag_verified": tag_ok,
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
        run(git_command("clone", "--filter=blob:none", "--no-checkout", "--no-tags",
                        spec["official_origin"], str(destination)))
    origin = run(git_command("remote", "get-url", "origin"), cwd=destination)
    if origin.rstrip("/") != spec["official_origin"].rstrip("/"):
        raise RuntimeError(f"refusing unexpected origin for {name}: {origin}")
    if spec.get("tag"):
        tag = spec["tag"]
        run(
            git_command(
                "fetch", "--depth", "1", "origin",
                f"refs/tags/{tag}:refs/tags/{tag}",
            ),
            cwd=destination,
        )
    else:
        run(
            git_command("fetch", "--depth", "1", "origin", spec["revision"]),
            cwd=destination,
        )
    run(git_command("checkout", "--detach", spec["revision"]), cwd=destination)
    return verify_git(name, destination)


def verify_font_archive(
    name: str,
    cache: pathlib.Path,
    *,
    allow_machine_cache: bool = False,
) -> int:
    spec = component(name)
    if spec.get("kind") != "font-archive":
        raise RuntimeError(f"{name} is not a font archive")
    destination = controlled_destination(
        name,
        cache.resolve(),
        allow_machine_cache=allow_machine_cache,
    )
    actual_members: dict[str, dict[str, str]] = {}
    ok = destination.is_dir()
    for relative_name, member in spec["members"].items():
        path = destination / relative_name
        if path.parent != destination or pathlib.PurePosixPath(relative_name).name != relative_name:
            raise RuntimeError(f"unsafe admitted font member name: {relative_name}")
        actual_sha = sha256_file(path) if path.is_file() else "MISSING"
        actual_members[relative_name] = {
            "sha256": actual_sha,
            "expected_sha256": member["sha256"],
        }
        ok = ok and actual_sha == member["sha256"]
    print(json.dumps({
        "component": name,
        "source": str(destination),
        "official_origin": spec["official_origin"],
        "release_tag": spec["release_tag"],
        "members": actual_members,
        "verified": ok,
    }, indent=2))
    return 0 if ok else 2


def sync_font_archive(name: str, cache: pathlib.Path, fresh: bool) -> int:
    spec = component(name)
    if spec.get("kind") != "font-archive":
        raise RuntimeError(f"{name} cannot be synced as a font archive")
    cache = cache.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    destination = controlled_destination(name, cache)
    if fresh and destination.exists():
        shutil.rmtree(destination)
    if destination.exists():
        return verify_font_archive(name, cache)

    temporary = cache / f".{name}.download"
    if temporary.exists():
        temporary.unlink()
    try:
        with urllib.request.urlopen(spec["archive_url"]) as response, temporary.open("wb") as output:
            shutil.copyfileobj(response, output)
        if sha256_file(temporary) != spec["archive_sha256"]:
            raise RuntimeError(f"downloaded archive digest mismatch for {name}")
        destination.mkdir()
        with zipfile.ZipFile(temporary) as archive:
            names = set(archive.namelist())
            for relative_name, member in spec["members"].items():
                archive_path = member["archive_path"]
                if archive_path not in names:
                    raise RuntimeError(
                        f"admitted member is missing from {name}: {archive_path}"
                    )
                payload = archive.read(archive_path)
                if hashlib.sha256(payload).hexdigest() != member["sha256"]:
                    raise RuntimeError(
                        f"admitted member digest mismatch for {name}: {archive_path}"
                    )
                (destination / relative_name).write_bytes(payload)
    except Exception:
        if destination.exists():
            shutil.rmtree(destination)
        raise
    finally:
        if temporary.exists():
            temporary.unlink()
    return verify_font_archive(name, cache)


def git_dependency_inventory(root: pathlib.Path) -> list[dict[str, str]]:
    inventory: list[dict[str, str]] = []
    externals = root / "third_party" / "externals"
    if not externals.is_dir():
        return inventory
    for dependency in sorted(path for path in externals.iterdir() if path.is_dir()):
        try:
            head = run(git_command("rev-parse", "HEAD"), cwd=dependency)
            origin = run(git_command("remote", "get-url", "origin"), cwd=dependency)
            dirty = run(git_command("status", "--porcelain"), cwd=dependency)
        except RuntimeError as error:
            raise RuntimeError(
                f"Skia dependency is not a verifiable Git checkout: {dependency}"
            ) from error
        if dirty:
            raise RuntimeError(f"Skia dependency worktree is modified: {dependency}")
        inventory.append({
            "path": dependency.relative_to(root).as_posix(),
            "origin": origin,
            "revision": head,
            "clean": True,
        })
    return inventory


def verify_skia_materialization(
    cache: pathlib.Path,
    emit: bool = True,
    *,
    allow_machine_cache: bool = False,
    allow_compatible_checkout: bool = False,
) -> dict:
    cache = cache.resolve()
    skia = controlled_destination(
        "skia",
        cache,
        allow_machine_cache=allow_machine_cache,
        allow_compatible_checkout=allow_compatible_checkout,
    )
    depot_tools = controlled_destination(
        "depot_tools",
        cache,
        allow_machine_cache=allow_machine_cache,
        allow_compatible_checkout=allow_compatible_checkout,
    )
    if (
        verify_git("skia", skia, emit=emit) != 0
        or verify_git("depot_tools", depot_tools, emit=emit) != 0
    ):
        raise RuntimeError("Skia roots do not match the foundation lock")
    dependency_record = skia_dependency_record_path(cache)
    if not dependency_record.is_file():
        raise RuntimeError("Skia dependency record is missing; run hydrate-skia")
    record = json.loads(dependency_record.read_text(encoding="utf-8"))
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
        "record": str(dependency_record),
        "record_sha256": sha256_file(dependency_record),
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
        "path": path.relative_to(ROOT).as_posix(),
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
    if os.name == "nt":
        emsdk_environment = (
            skia / "third_party" / "externals" / "emsdk" / "emsdk_set_env.bat"
        )
        if emsdk_environment.is_file():
            emsdk_environment.unlink()

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
                "path": path.relative_to(skia).as_posix(),
                "sha256": sha256_file(path),
            }
            for name, path in {
                "gn": skia / "bin" / ("gn.exe" if os.name == "nt" else "gn"),
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


def ffmpeg_profile_document() -> dict:
    return json.loads(FFMPEG_PROFILES.read_text(encoding="utf-8"))


def ffmpeg_profiles() -> dict:
    return ffmpeg_profile_document()["profiles"]


def ffmpeg_workspace_alias() -> pathlib.Path:
    """Return a no-space alias for upstream configure scripts that reject spaces."""
    digest = hashlib.sha256(str(ROOT.resolve()).encode("utf-8")).hexdigest()[:12]
    alias = pathlib.Path(tempfile.gettempdir()) / f"refusion-ffmpeg-{digest}"
    if alias.exists() or alias.is_symlink():
        if not alias.is_symlink() or alias.resolve() != ROOT.resolve():
            raise RuntimeError(f"refusing unexpected FFmpeg workspace alias: {alias}")
    else:
        alias.symlink_to(ROOT.resolve(), target_is_directory=True)
    return alias


def ffmpeg_enabled_components(config_path: pathlib.Path) -> dict[str, list[str]]:
    if not config_path.is_file():
        raise RuntimeError(f"FFmpeg component configuration is missing: {config_path}")
    categories = {
        "DECODER",
        "ENCODER",
        "DEMUXER",
        "MUXER",
        "FILTER",
        "BSF",
        "PARSER",
        "PROTOCOL",
    }
    enabled: dict[str, list[str]] = {category: [] for category in categories}
    pattern = re.compile(r"^#define CONFIG_([A-Z0-9_]+)_([A-Z]+) 1$")
    for line in config_path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line)
        if match and match.group(2) in enabled:
            enabled[match.group(2)].append(match.group(1))
    for values in enabled.values():
        values.sort()
    return dict(sorted(enabled.items()))


def verify_ffmpeg_allowlist(config_path: pathlib.Path) -> dict[str, list[str]]:
    enabled = ffmpeg_enabled_components(config_path)
    receipt = ffmpeg_profile_document()["allowlist_receipt"]
    expected_demuxers = sorted(receipt["required_enabled_demuxers"])
    if enabled["DEMUXER"] != expected_demuxers:
        raise RuntimeError(
            f"FFmpeg demuxer allowlist drifted: {enabled['DEMUXER']} != {expected_demuxers}"
        )
    for category in receipt["forbidden_enabled_categories"]:
        if enabled[category]:
            raise RuntimeError(
                f"FFmpeg forbidden {category.lower()} components are enabled: "
                f"{enabled[category]}"
            )
    return enabled


def build_ffmpeg(
    profile_name: str,
    cache: pathlib.Path,
    build_root: pathlib.Path,
    fresh: bool = False,
) -> int:
    profiles = ffmpeg_profiles()
    if profile_name not in profiles:
        raise RuntimeError(f"unknown FFmpeg build profile: {profile_name}")
    profile = profiles[profile_name]
    if profile["host"] != host_key():
        raise RuntimeError(
            f"FFmpeg profile {profile_name} requires {profile['host']}, host is {host_key()}"
        )

    cache = cache.resolve()
    source = controlled_destination("ffmpeg", cache)
    if verify_git("ffmpeg", source) != 0:
        raise RuntimeError("refusing to build unverified FFmpeg sources")
    if build_root.resolve() != FFMPEG_BUILD_ROOT.resolve():
        raise RuntimeError(f"FFmpeg builds must remain inside ReFusion: {FFMPEG_BUILD_ROOT}")

    output = build_root.resolve() / profile_name
    if fresh and output.exists():
        if output.parent != FFMPEG_BUILD_ROOT.resolve():
            raise RuntimeError(f"refusing uncontrolled FFmpeg build cleanup: {output}")
        shutil.rmtree(output)
    output.mkdir(parents=True, exist_ok=True)
    install = output / "install"
    workspace_alias = ffmpeg_workspace_alias()
    source_alias = workspace_alias / source.relative_to(ROOT)
    output_alias = workspace_alias / output.relative_to(ROOT)
    install_alias = workspace_alias / install.relative_to(ROOT)
    configure_args = [*profile["configure_args"], f"--prefix={install_alias}"]
    receipt_configure_args = [
        argument.replace(str(workspace_alias), "$REFUSION_ROOT")
        for argument in configure_args
    ]
    configure = source_alias / "configure"

    if os.name == "nt":
        bash_value = os.environ.get("REFUSION_MSYS2_BASH")
        if not bash_value:
            raise RuntimeError(
                "REFUSION_MSYS2_BASH must name the pinned MSYS2 bash for the MSVC profile"
            )
        bash = pathlib.Path(bash_value)
        if not bash.is_file():
            raise RuntimeError(f"pinned MSYS2 bash is missing: {bash}")
        run([str(bash), configure.as_posix(), *configure_args], cwd=output_alias)
        jobs = str(max(1, os.cpu_count() or 1))
        run([str(bash), "-c", f"make -j{jobs}"], cwd=output_alias)
        run([str(bash), "-c", "make install"], cwd=output_alias)
    elif sys.platform == "darwin":
        run([str(configure), *configure_args], cwd=output_alias)
        run(["/usr/bin/make", f"-j{max(1, os.cpu_count() or 1)}"], cwd=output_alias)
        run(["/usr/bin/make", "install"], cwd=output_alias)
    else:
        raise RuntimeError("no admitted FFmpeg demux profile for this host")

    config_path = output / "config_components.h"
    enabled = verify_ffmpeg_allowlist(config_path)
    library_patterns = ("*.dll", "*.lib") if os.name == "nt" else ("*.dylib",)
    libraries = sorted(
        path
        for pattern in library_patterns
        for path in (install / "lib").glob(pattern)
        if path.is_file()
    )
    if not libraries:
        raise RuntimeError("FFmpeg shared-library installation is empty")
    record = {
        "schema_version": 1,
        "profile": profile_name,
        "source_origin": component("ffmpeg")["official_origin"],
        "source_tag": component("ffmpeg")["tag"],
        "source_revision": component("ffmpeg")["revision"],
        "configure_args": receipt_configure_args,
        "configure_args_sha256": hashlib.sha256(
            ("\n".join(receipt_configure_args) + "\n").encode("utf-8")
        ).hexdigest(),
        "config_components_sha256": sha256_file(config_path),
        "enabled_components": enabled,
        "libraries": [
            {
                "path": path.relative_to(ROOT).as_posix(),
                "size": path.stat().st_size,
                "sha256": sha256_file(path),
            }
            for path in libraries
        ],
        "host": {
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
        },
    }
    write_json_atomic(output / "refusion-build.json", record)
    print(json.dumps(record, indent=2))
    return 0


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

    gn = skia / "bin" / ("gn.exe" if os.name == "nt" else "gn")
    ninja = (skia / "third_party" / "ninja" /
             ("ninja.exe" if os.name == "nt" else "ninja"))
    if not gn.is_file() or not ninja.is_file():
        raise RuntimeError("Skia GN/Ninja tools are missing; run hydrate-skia first")

    args_path = SKIA_PROFILES.parent / profile["gn_args"]
    args = args_path.read_text(encoding="utf-8")
    source_patch = profile.get("source_patch")
    source_patch_path = ROOT / source_patch if source_patch else None
    if source_patch_path is not None and not source_patch_path.is_file():
        raise RuntimeError(f"Skia source patch is missing: {source_patch_path}")
    patched_file_times: dict[pathlib.Path, tuple[int, int]] = {}
    if source_patch_path is not None:
        for line in source_patch_path.read_text(encoding="utf-8").splitlines():
            if not line.startswith("+++ b/"):
                continue
            patched_file = (skia / line.removeprefix("+++ b/")).resolve()
            if not patched_file.is_relative_to(skia) or not patched_file.is_file():
                raise RuntimeError(
                    f"Skia patch references an invalid source: {patched_file}"
                )
            stat = patched_file.stat()
            patched_file_times[patched_file] = (stat.st_atime_ns, stat.st_mtime_ns)
    if build_root.resolve() != SKIA_BUILD_ROOT.resolve():
        raise RuntimeError(f"Skia builds must remain inside ReFusion: {SKIA_BUILD_ROOT}")
    output = build_root.resolve() / profile_name
    output.mkdir(parents=True, exist_ok=True)
    recorded_patch_matches = False
    previous_record_path = output / "refusion-build.json"
    if source_patch_path is not None and previous_record_path.is_file():
        try:
            previous_record = json.loads(
                previous_record_path.read_text(encoding="utf-8")
            )
            recorded_patch_matches = (
                previous_record.get("profile") == profile_name
                and previous_record.get("source_revision")
                == component("skia")["revision"]
                and previous_record.get("gn_args_sha256") == sha256_file(args_path)
                and previous_record.get("source_patch_sha256")
                == sha256_file(source_patch_path)
            )
        except (json.JSONDecodeError, OSError):
            recorded_patch_matches = False
    patch_applied = False
    try:
        if source_patch_path is not None:
            run(git_command("apply", "--check", str(source_patch_path)), cwd=skia)
            run(git_command("apply", str(source_patch_path)), cwd=skia)
            patch_applied = True
            if recorded_patch_matches:
                for patched_file, times in patched_file_times.items():
                    os.utime(patched_file, ns=times)
        run([str(gn), "gen", str(output), f"--args={args}"], cwd=skia)
        run([str(ninja), "-C", str(output), *profile["targets"]], cwd=skia)
    finally:
        if patch_applied:
            run(git_command("apply", "--reverse", str(source_patch_path)), cwd=skia)
            for patched_file, times in patched_file_times.items():
                os.utime(patched_file, ns=times)
    if verify_git("skia", skia) != 0:
        raise RuntimeError("Skia source patch cleanup did not restore clean sources")

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
        "path": bundle.relative_to(ROOT).as_posix(),
        "size": bundle.stat().st_size,
        "sha256": sha256_file(bundle),
    }
    archive_records = [
        {
            "path": path.relative_to(ROOT).as_posix(),
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
        "gn_args": args_path.relative_to(ROOT).as_posix(),
        "gn_args_sha256": sha256_file(args_path),
        "source_patch": source_patch,
        "source_patch_sha256": (
            sha256_file(source_patch_path) if source_patch_path else None
        ),
        "dependency_record": SKIA_DEPENDENCY_RECORD.relative_to(ROOT).as_posix(),
        "dependency_record_sha256": materialization["record_sha256"],
        "tracked_dependency_lock": pathlib.Path(
            materialization["tracked_lock"]
        ).relative_to(ROOT).as_posix(),
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


def machine_build_is_admitted(
    build_dir: pathlib.Path,
    profile_name: str,
    root: pathlib.Path | None = None,
) -> bool:
    build_dir = build_dir.resolve()
    root = (root or default_machine_cache_root()).resolve()
    try:
        relative = build_dir.relative_to(root)
    except ValueError:
        return False
    parts = relative.parts
    return (
        len(parts) == 4
        and parts[0] == "a"
        and parts[1] == "skia"
        and parts[2] == profile_name
        and is_cache_key(parts[3])
    )


def verify_skia_build(
    profile_name: str,
    cache: pathlib.Path,
    build_dir: pathlib.Path,
    *,
    allow_machine_cache: bool = False,
    allow_compatible_checkout: bool = False,
    emit: bool = True,
) -> dict:
    profiles = skia_profiles()
    if profile_name not in profiles:
        raise RuntimeError(f"unknown Skia build profile: {profile_name}")
    profile = profiles[profile_name]
    cache = cache.resolve()
    build_dir = build_dir.resolve()
    local_build = build_dir == (SKIA_BUILD_ROOT / profile_name).resolve()
    compatible_build = (
        allow_compatible_checkout
        and build_dir.name == profile_name
        and build_dir.parent.name == "skia"
        and build_dir.parent.parent.name == "deps-build"
        and build_dir.parent.parent.parent.name == "out"
        and checkout_source_cache_is_compatible(
            build_dir.parent.parent.parent / "deps-src"
        )
    )
    cached_build = allow_machine_cache and machine_build_is_admitted(
        build_dir, profile_name
    )
    if not local_build and not compatible_build and not cached_build:
        raise RuntimeError(
            "Skia build must be local, from a compatible checkout selected for "
            "publication, or from a verified ReFusion machine cache"
        )

    materialization = verify_skia_materialization(
        cache,
        emit=False,
        allow_machine_cache=allow_machine_cache,
        allow_compatible_checkout=allow_compatible_checkout,
    )
    record_path = build_dir / "refusion-build.json"
    if not record_path.is_file():
        raise RuntimeError(f"Skia build record is missing: {record_path}")
    record = json.loads(record_path.read_text(encoding="utf-8"))
    expected_patch = profile.get("source_patch")
    expected_values = {
        "schema_version": 2,
        "profile": profile_name,
        "source_origin": component("skia")["official_origin"],
        "source_revision": component("skia")["revision"],
        "gn_args": (SKIA_PROFILES.parent / profile["gn_args"])
        .relative_to(ROOT)
        .as_posix(),
        "source_patch": expected_patch,
        "dependency_record": "out/deps-src/skia-dependencies.lock.json",
        "tracked_dependency_lock": tracked_transitive_lock_path()
        .relative_to(ROOT)
        .as_posix(),
        "dependency_count": materialization["dependency_count"],
        "targets": profile["targets"],
    }
    for name, expected in expected_values.items():
        if record.get(name) != expected:
            raise RuntimeError(
                f"Skia build record {name} mismatch: {record.get(name)!r} != {expected!r}"
            )

    args_path = SKIA_PROFILES.parent / profile["gn_args"]
    if not recorded_text_sha256_matches(args_path, record.get("gn_args_sha256")):
        raise RuntimeError("Skia build GN profile digest changed")
    patch_matches = (
        record.get("source_patch_sha256") is None
        if expected_patch is None
        else recorded_text_sha256_matches(
            ROOT / expected_patch, record.get("source_patch_sha256")
        )
    )
    if not patch_matches:
        raise RuntimeError("Skia build source-patch digest changed")
    dependency_record = skia_dependency_record_path(cache)
    if not recorded_text_sha256_matches(
        dependency_record, record.get("dependency_record_sha256")
    ):
        raise RuntimeError("Skia build dependency-record digest changed")
    if not recorded_text_sha256_matches(
        tracked_transitive_lock_path(),
        record.get("tracked_dependency_lock_sha256"),
    ):
        raise RuntimeError("Skia build tracked-lock digest changed")

    expected_system = platform.system()
    actual_host = record.get("host", {})
    if actual_host.get("system") != expected_system:
        raise RuntimeError(
            f"Skia build host mismatch: {actual_host.get('system')} != {expected_system}"
        )
    actual_machine = str(actual_host.get("machine", "")).lower()
    current_machine = platform.machine().lower()
    machine_aliases = (
        {"amd64", "x86_64"}
        if current_machine in {"amd64", "x86_64"}
        else {current_machine}
    )
    if actual_machine not in machine_aliases:
        raise RuntimeError(
            f"Skia build architecture mismatch: {actual_machine} != {current_machine}"
        )

    artifact_record = record.get("artifact", {})
    artifact = build_dir / profile["bundle_artifact"]
    recorded_artifact = pathlib.PurePosixPath(
        str(artifact_record.get("path", ""))
    )
    if recorded_artifact.name != artifact.name:
        raise RuntimeError("Skia build artifact identity changed")
    if not artifact.is_file():
        raise RuntimeError(f"Skia build artifact is missing: {artifact}")
    if artifact.stat().st_size != artifact_record.get("size"):
        raise RuntimeError("Skia build artifact size changed")
    artifact_sha = sha256_file(artifact)
    if artifact_sha != artifact_record.get("sha256"):
        raise RuntimeError("Skia build artifact digest changed")

    runtime_files: dict[str, dict[str, str | int]] = {}
    if profile_name == "windows-x64-d3d12":
        icu_data = build_dir / "icudtl.dat"
        source_icu_data = cache / "skia" / "third_party" / "externals" / "icu" / "common" / "icudtl.dat"
        if not icu_data.is_file() or not source_icu_data.is_file():
            raise RuntimeError("Skia Windows ICU runtime data is missing")
        icu_sha = sha256_file(icu_data)
        if icu_sha != sha256_file(source_icu_data):
            raise RuntimeError("Skia Windows ICU runtime data changed")
        runtime_files["icudtl.dat"] = {
            "size": icu_data.stat().st_size,
            "sha256": icu_sha,
        }

    result = {
        "verified": True,
        "profile": profile_name,
        "source_cache": str(cache),
        "build_dir": str(build_dir),
        "build_record": str(record_path),
        "build_record_sha256": sha256_file(record_path),
        "artifact": {
            "path": str(artifact),
            "size": artifact.stat().st_size,
            "sha256": artifact_sha,
        },
        "runtime_files": runtime_files,
        "materialization": materialization,
    }
    if emit:
        print(json.dumps(result, indent=2))
    return result


def source_cache_identity(materialization: dict) -> tuple[str, dict]:
    value = {
        "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
        "host": host_key(),
        "skia_revision": component("skia")["revision"],
        "depot_tools_revision": component("depot_tools")["revision"],
        "dependency_record_sha256": materialization["record_sha256"],
        "tracked_dependency_lock_sha256": materialization["tracked_lock_sha256"],
    }
    return canonical_sha256(value), value


def artifact_cache_identity(verification: dict) -> tuple[str, dict]:
    value = {
        "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
        "host": host_key(),
        "profile": verification["profile"],
        "build_record_sha256": verification["build_record_sha256"],
        "artifact_sha256": verification["artifact"]["sha256"],
        "dependency_record_sha256": verification["materialization"]["record_sha256"],
        "tracked_dependency_lock_sha256": verification["materialization"]["tracked_lock_sha256"],
    }
    return canonical_sha256(value), value


def safe_cache_index_path(root: pathlib.Path, relative: str) -> pathlib.Path:
    if not isinstance(relative, str):
        raise RuntimeError("machine-cache index path must be a string")
    candidate = (root / pathlib.PurePosixPath(relative)).resolve()
    try:
        candidate.relative_to(root.resolve())
    except ValueError as error:
        raise RuntimeError(f"machine-cache index escapes its root: {relative}") from error
    return candidate


def read_cache_receipt(path: pathlib.Path, kind: str) -> dict:
    if not path.is_file():
        raise RuntimeError(f"machine-cache receipt is missing: {path}")
    value = json.loads(path.read_text(encoding="utf-8"))
    if (
        not isinstance(value, dict)
        or value.get("schema_version") != MACHINE_CACHE_SCHEMA_VERSION
        or value.get("kind") != kind
    ):
        raise RuntimeError(f"machine-cache receipt is malformed: {path}")
    return value


def require_cache_identity(
    actual: object,
    expected_key: str,
    diagnostic: str,
) -> None:
    if not is_sha256(actual) or actual != expected_key:
        raise RuntimeError(diagnostic)


def publish_skia_machine_cache(
    profile_name: str,
    source_checkout: pathlib.Path,
    machine_root: pathlib.Path,
) -> int:
    os.environ["REFUSION_MACHINE_CACHE_ROOT"] = str(machine_root.resolve())
    source_checkout = source_checkout.resolve()
    source_cache = source_checkout / "out" / "deps-src"
    build_dir = source_checkout / "out" / "deps-build" / "skia" / profile_name
    verification = verify_skia_build(
        profile_name,
        source_cache,
        build_dir,
        allow_compatible_checkout=True,
        emit=False,
    )
    source_key, source_identity = source_cache_identity(
        verification["materialization"]
    )
    artifact_key, artifact_identity = artifact_cache_identity(verification)
    machine_root = machine_root.resolve()
    cached_source_parent = machine_root / "s" / source_key[:32]
    cached_source = cached_source_parent / "d"
    cached_build = (
        machine_root / "a" / "skia" / profile_name / artifact_key[:32]
    )

    if not cached_source.exists():
        copy_tree_atomic(source_cache, cached_source, machine_root / "t")
        write_json_atomic(
            cached_source_parent / "receipt.json",
            {
                "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
                "kind": "refusion-skia-source-cache",
                "identity": source_identity,
                "source_cache": "d",
            },
        )
    verify_skia_materialization(
        cached_source, emit=False, allow_machine_cache=True
    )
    source_receipt = read_cache_receipt(
        cached_source_parent / "receipt.json", "refusion-skia-source-cache"
    )
    if source_receipt.get("identity") != source_identity:
        raise RuntimeError("machine-cache Skia source receipt identity changed")

    if not cached_build.exists():
        cached_build.mkdir(parents=True)
        try:
            source_build = pathlib.Path(verification["build_dir"])
            profile = skia_profiles()[profile_name]
            for name in ("refusion-build.json", profile["bundle_artifact"]):
                copy_file_atomic(source_build / name, cached_build / name)
            for name in verification["runtime_files"]:
                copy_file_atomic(source_build / name, cached_build / name)
            write_json_atomic(
                cached_build / "cache-receipt.json",
                {
                    "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
                    "kind": "refusion-skia-build-cache",
                    "identity": artifact_identity,
                    "source_key": source_key,
                },
            )
        except Exception:
            shutil.rmtree(cached_build, ignore_errors=True)
            raise
    verify_skia_build(
        profile_name,
        cached_source,
        cached_build,
        allow_machine_cache=True,
        emit=False,
    )
    artifact_receipt = read_cache_receipt(
        cached_build / "cache-receipt.json", "refusion-skia-build-cache"
    )
    if (
        artifact_receipt.get("identity") != artifact_identity
        or artifact_receipt.get("source_key") != source_key
    ):
        raise RuntimeError("machine-cache Skia artifact receipt identity changed")

    index = read_machine_cache_index(machine_root)
    index["skia_profiles"][profile_name] = {
        "host": host_key(),
        "source_key": source_key,
        "artifact_key": artifact_key,
        "source_cache": cached_source.relative_to(machine_root).as_posix(),
        "build_dir": cached_build.relative_to(machine_root).as_posix(),
    }
    write_json_atomic(machine_cache_index_path(machine_root), index)
    print(json.dumps({
        "published": True,
        "profile": profile_name,
        "machine_cache_root": str(machine_root),
        "source_key": source_key,
        "artifact_key": artifact_key,
        "source_cache": str(cached_source),
        "build_dir": str(cached_build),
    }, indent=2))
    return 0


def resolve_skia_machine_cache(
    profile_name: str,
    machine_root: pathlib.Path,
    *,
    emit: bool = True,
) -> dict:
    machine_root = machine_root.resolve()
    os.environ["REFUSION_MACHINE_CACHE_ROOT"] = str(machine_root)
    index = read_machine_cache_index(machine_root)
    entry = index["skia_profiles"].get(profile_name)
    if not isinstance(entry, dict) or entry.get("host") != host_key():
        raise RuntimeError(
            f"no verified {profile_name} entry exists in {machine_root}"
        )
    source_cache = safe_cache_index_path(machine_root, entry["source_cache"])
    build_dir = safe_cache_index_path(machine_root, entry["build_dir"])
    if not machine_source_cache_is_admitted(source_cache, machine_root):
        raise RuntimeError("machine-cache Skia source path is not admitted")
    if not machine_build_is_admitted(build_dir, profile_name, machine_root):
        raise RuntimeError("machine-cache Skia build path is not admitted")
    verification = verify_skia_build(
        profile_name,
        source_cache,
        build_dir,
        allow_machine_cache=True,
        emit=False,
    )
    source_key, source_identity = source_cache_identity(
        verification["materialization"]
    )
    artifact_key, artifact_identity = artifact_cache_identity(verification)
    require_cache_identity(
        entry.get("source_key"), source_key, "machine-cache Skia source key changed"
    )
    require_cache_identity(
        entry.get("artifact_key"),
        artifact_key,
        "machine-cache Skia artifact key changed",
    )
    if source_cache.parent.name != source_key[:32]:
        raise RuntimeError("machine-cache Skia source directory key changed")
    if build_dir.name != artifact_key[:32]:
        raise RuntimeError("machine-cache Skia artifact directory key changed")
    source_receipt = read_cache_receipt(
        source_cache.parent / "receipt.json", "refusion-skia-source-cache"
    )
    artifact_receipt = read_cache_receipt(
        build_dir / "cache-receipt.json", "refusion-skia-build-cache"
    )
    if source_receipt.get("identity") != source_identity:
        raise RuntimeError("machine-cache Skia source receipt identity changed")
    if (
        artifact_receipt.get("identity") != artifact_identity
        or artifact_receipt.get("source_key") != source_key
    ):
        raise RuntimeError("machine-cache Skia artifact receipt identity changed")
    result = {
        "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
        "machine_cache_root": str(machine_root),
        "profile": profile_name,
        "source_cache": str(source_cache),
        "skia_source": str(source_cache / "skia"),
        "skia_build": str(build_dir),
        "artifact_sha256": verification["artifact"]["sha256"],
        "dependency_record_sha256": verification["materialization"]["record_sha256"],
        "tracked_dependency_lock_sha256": verification["materialization"]["tracked_lock_sha256"],
    }
    if emit:
        print(json.dumps(result, indent=2))
    return result


def qtpaths_executable(qt_root: pathlib.Path) -> pathlib.Path:
    names = ("qtpaths.exe", "qtpaths6.exe") if os.name == "nt" else ("qtpaths", "qtpaths6")
    for name in names:
        candidate = qt_root / "bin" / name
        if candidate.is_file():
            return candidate
    raise RuntimeError(f"Qt paths tool is missing below {qt_root}")


def verify_qt_sdk(qt_root: pathlib.Path) -> dict:
    qt_root = qt_root.resolve()
    qtpaths = qtpaths_executable(qt_root)
    expected_version = component("qt")["version"]
    actual_version = run([str(qtpaths), "--query", "QT_VERSION"])
    if actual_version != expected_version:
        raise RuntimeError(
            f"Qt SDK version mismatch: {actual_version} != {expected_version}"
        )
    required_modules = (
        "Core", "Gui", "Qml", "Quick", "QuickControls2", "QuickDialogs2"
    )
    identity_files = [qtpaths, qt_root / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake"]
    identity_files.extend(
        qt_root / "lib" / "cmake" / f"Qt6{name}" / f"Qt6{name}Config.cmake"
        for name in required_modules
    )
    missing = [path for path in identity_files if not path.is_file()]
    if missing:
        raise RuntimeError(
            "Qt SDK is missing required module metadata: "
            + ", ".join(str(path) for path in missing)
        )
    files = {
        path.relative_to(qt_root).as_posix(): sha256_file(path)
        for path in identity_files
    }
    identity = {
        "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
        "host": host_key(),
        "version": actual_version,
        "required_modules": list(required_modules),
        "identity_files": files,
    }
    return {
        "verified": True,
        "root": str(qt_root),
        "version": actual_version,
        "identity": identity,
        "identity_sha256": canonical_sha256(identity),
    }


def publish_qt_machine_cache(
    source: pathlib.Path,
    machine_root: pathlib.Path,
) -> int:
    verification = verify_qt_sdk(source)
    machine_root = machine_root.resolve()
    identity_sha = verification["identity_sha256"]
    destination = (
        machine_root
        / "q"
        / host_key()
        / verification["version"]
        / identity_sha[:32]
    )
    if not destination.exists():
        copy_tree_atomic(source.resolve(), destination, machine_root / "t")
        write_json_atomic(
            destination / "cache-receipt.json",
            {
                "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
                "kind": "refusion-qt-sdk-cache",
                "official_origin": component("qt")["official_origin"],
                "identity": verification["identity"],
                "identity_sha256": identity_sha,
                "release_qualified": False,
            },
        )
    cached_verification = verify_qt_sdk(destination)
    if cached_verification["identity_sha256"] != identity_sha:
        raise RuntimeError("copied Qt SDK identity changed")
    receipt = read_cache_receipt(
        destination / "cache-receipt.json", "refusion-qt-sdk-cache"
    )
    if (
        receipt.get("official_origin") != component("qt")["official_origin"]
        or receipt.get("identity") != verification["identity"]
        or receipt.get("identity_sha256") != identity_sha
        or receipt.get("release_qualified") is not False
    ):
        raise RuntimeError("machine-cache Qt receipt identity changed")
    index = read_machine_cache_index(machine_root)
    index["qt_sdks"][host_key()] = {
        "version": verification["version"],
        "identity_sha256": identity_sha,
        "path": destination.relative_to(machine_root).as_posix(),
        "release_qualified": False,
    }
    write_json_atomic(machine_cache_index_path(machine_root), index)
    print(json.dumps({
        "published": True,
        "machine_cache_root": str(machine_root),
        "qt_root": str(destination),
        "version": verification["version"],
        "identity_sha256": identity_sha,
        "release_qualified": False,
    }, indent=2))
    return 0


def resolve_qt_machine_cache(
    machine_root: pathlib.Path,
    *,
    emit: bool = True,
) -> dict:
    machine_root = machine_root.resolve()
    entry = read_machine_cache_index(machine_root)["qt_sdks"].get(host_key())
    if not isinstance(entry, dict):
        raise RuntimeError(f"no verified Qt SDK exists in {machine_root}")
    qt_root = safe_cache_index_path(machine_root, entry["path"])
    verification = verify_qt_sdk(qt_root)
    identity_sha = verification["identity_sha256"]
    require_cache_identity(
        entry.get("identity_sha256"),
        identity_sha,
        "machine-cache Qt SDK identity changed",
    )
    if (
        entry.get("version") != verification["version"]
        or entry.get("release_qualified") is not False
        or qt_root.name != identity_sha[:32]
    ):
        raise RuntimeError("machine-cache Qt index identity changed")
    receipt = read_cache_receipt(
        qt_root / "cache-receipt.json", "refusion-qt-sdk-cache"
    )
    if (
        receipt.get("official_origin") != component("qt")["official_origin"]
        or receipt.get("identity") != verification["identity"]
        or receipt.get("identity_sha256") != identity_sha
        or receipt.get("release_qualified") is not False
    ):
        raise RuntimeError("machine-cache Qt receipt identity changed")
    result = {
        "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
        "machine_cache_root": str(machine_root),
        "qt_root": str(qt_root),
        "version": verification["version"],
        "identity_sha256": verification["identity_sha256"],
        "release_qualified": False,
    }
    if emit:
        print(json.dumps(result, indent=2))
    return result


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
    sync_fonts = sub.add_parser("sync-font")
    sync_fonts.add_argument("component")
    sync_fonts.add_argument("--fresh", action="store_true")
    verify_fonts = sub.add_parser("verify-font")
    verify_fonts.add_argument("component")
    verify_fonts.add_argument("--source-cache", type=pathlib.Path)
    sub.add_parser("hydrate-skia")
    verify_skia = sub.add_parser("verify-skia-materialization")
    verify_skia.add_argument("--source-cache", type=pathlib.Path)
    sub.add_parser("lock-skia-materialization")
    build = sub.add_parser("build-skia")
    build.add_argument("--profile", required=True, choices=tuple(skia_profiles()))
    build_ffmpeg_parser = sub.add_parser("build-ffmpeg")
    build_ffmpeg_parser.add_argument(
        "--profile", required=True, choices=tuple(ffmpeg_profiles())
    )
    build_ffmpeg_parser.add_argument("--fresh", action="store_true")
    verify_build = sub.add_parser("verify-skia-build")
    verify_build.add_argument("--profile", required=True, choices=tuple(skia_profiles()))
    verify_build.add_argument("--source-cache", required=True, type=pathlib.Path)
    verify_build.add_argument("--build-dir", required=True, type=pathlib.Path)
    machine_cache = sub.add_parser("machine-cache")
    machine_cache.add_argument("--root", type=pathlib.Path)
    cache_sub = machine_cache.add_subparsers(dest="cache_command", required=True)
    cache_publish_skia = cache_sub.add_parser("publish-skia")
    cache_publish_skia.add_argument(
        "--profile", required=True, choices=tuple(skia_profiles())
    )
    cache_publish_skia.add_argument(
        "--from-checkout", required=True, type=pathlib.Path
    )
    cache_resolve_skia = cache_sub.add_parser("resolve-skia")
    cache_resolve_skia.add_argument(
        "--profile", required=True, choices=tuple(skia_profiles())
    )
    cache_publish_qt = cache_sub.add_parser("publish-qt")
    cache_publish_qt.add_argument("--source", required=True, type=pathlib.Path)
    cache_sub.add_parser("resolve-qt")
    cache_sub.add_parser("status")
    args = parser.parse_args()
    try:
        if args.command == "doctor":
            return doctor()
        if args.command == "verify":
            return verify_git(args.component, args.source.resolve())
        if args.command == "sync":
            return sync_git(args.component, SOURCE_CACHE, args.fresh)
        if args.command == "sync-font":
            return sync_font_archive(args.component, SOURCE_CACHE, args.fresh)
        if args.command == "verify-font":
            source_cache = (args.source_cache or SOURCE_CACHE).resolve()
            return verify_font_archive(
                args.component,
                source_cache,
                allow_machine_cache=source_cache != SOURCE_CACHE.resolve(),
            )
        if args.command == "hydrate-skia":
            return hydrate_skia(SOURCE_CACHE)
        if args.command == "verify-skia-materialization":
            source_cache = (args.source_cache or SOURCE_CACHE).resolve()
            verify_skia_materialization(
                source_cache,
                allow_machine_cache=source_cache != SOURCE_CACHE.resolve(),
            )
            return 0
        if args.command == "lock-skia-materialization":
            return lock_skia_materialization(SOURCE_CACHE)
        if args.command == "build-skia":
            return build_skia(args.profile, SOURCE_CACHE, SKIA_BUILD_ROOT)
        if args.command == "build-ffmpeg":
            return build_ffmpeg(
                args.profile, SOURCE_CACHE, FFMPEG_BUILD_ROOT, args.fresh
            )
        if args.command == "verify-skia-build":
            verify_skia_build(
                args.profile,
                args.source_cache,
                args.build_dir,
                allow_machine_cache=True,
            )
            return 0
        machine_root = (args.root or default_machine_cache_root()).resolve()
        if args.cache_command == "publish-skia":
            return publish_skia_machine_cache(
                args.profile, args.from_checkout, machine_root
            )
        if args.cache_command == "resolve-skia":
            resolve_skia_machine_cache(args.profile, machine_root)
            return 0
        if args.cache_command == "publish-qt":
            return publish_qt_machine_cache(args.source, machine_root)
        if args.cache_command == "resolve-qt":
            resolve_qt_machine_cache(machine_root)
            return 0
        print(json.dumps({
            "schema_version": MACHINE_CACHE_SCHEMA_VERSION,
            "machine_cache_root": str(machine_root),
            "index": read_machine_cache_index(machine_root),
        }, indent=2))
        return 0
    except (RuntimeError, OSError, json.JSONDecodeError) as error:
        print(f"bootstrap error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
