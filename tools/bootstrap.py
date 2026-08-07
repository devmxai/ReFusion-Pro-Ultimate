#!/usr/bin/env python3
"""Pinned, explicit dependency bootstrap for ReFusion foundation."""

from __future__ import annotations

import argparse
import json
import pathlib
import shutil
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
LOCK = ROOT / "deps" / "manifest.lock.json"


def run(command: list[str], cwd: pathlib.Path | None = None) -> str:
    completed = subprocess.run(
        command, cwd=cwd, check=False, text=True, capture_output=True
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
    print(json.dumps({"lock": str(LOCK), "checks": checks}, indent=2))
    missing = [name for name, value in checks.items() if value.startswith("MISSING")]
    return 1 if any(name in {"git", "cmake", "ninja"} for name in missing) else 0


def component(name: str) -> dict:
    components = manifest()["components"]
    if name not in components:
        raise RuntimeError(f"unknown component: {name}")
    return components[name]


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
    ok = head == expected_head and origin.rstrip("/") == expected_origin.rstrip("/")
    print(json.dumps({
        "component": name,
        "source": str(source),
        "origin": origin,
        "expected_origin": expected_origin,
        "head": head,
        "expected_head": expected_head,
        "verified": ok,
    }, indent=2))
    return 0 if ok else 2


def sync_git(name: str, cache: pathlib.Path) -> int:
    spec = component(name)
    if spec.get("kind") != "git":
        raise RuntimeError(f"{name} cannot be synced automatically")
    cache.mkdir(parents=True, exist_ok=True)
    destination = cache / name
    if not destination.exists():
        run(["git", "clone", "--filter=blob:none", "--no-checkout",
             spec["official_origin"], str(destination)])
    origin = run(["git", "remote", "get-url", "origin"], cwd=destination)
    if origin.rstrip("/") != spec["official_origin"].rstrip("/"):
        raise RuntimeError(f"refusing unexpected origin for {name}: {origin}")
    run(["git", "fetch", "--depth", "1", "origin", spec["revision"]], cwd=destination)
    run(["git", "checkout", "--detach", spec["revision"]], cwd=destination)
    return verify_git(name, destination)


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("doctor")
    verify = sub.add_parser("verify")
    verify.add_argument("component")
    verify.add_argument("--source", required=True, type=pathlib.Path)
    sync = sub.add_parser("sync")
    sync.add_argument("component")
    sync.add_argument("--cache", type=pathlib.Path, default=ROOT / "out" / "deps-src")
    args = parser.parse_args()
    try:
        if args.command == "doctor":
            return doctor()
        if args.command == "verify":
            return verify_git(args.component, args.source.resolve())
        return sync_git(args.component, args.cache.resolve())
    except (RuntimeError, OSError, json.JSONDecodeError) as error:
        print(f"bootstrap error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

