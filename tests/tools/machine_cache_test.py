from __future__ import annotations

import importlib.util
import json
import os
import pathlib
import stat
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def load_bootstrap():
    specification = importlib.util.spec_from_file_location(
        "refusion_bootstrap_machine_cache", ROOT / "tools" / "bootstrap.py"
    )
    if specification is None or specification.loader is None:
        raise RuntimeError("unable to load tools/bootstrap.py")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def main() -> None:
    bootstrap = load_bootstrap()
    with tempfile.TemporaryDirectory(prefix="refusion-machine-cache-test-") as temporary:
        cache_root = pathlib.Path(temporary).resolve()
        previous = os.environ.get("REFUSION_MACHINE_CACHE_ROOT")
        os.environ["REFUSION_MACHINE_CACHE_ROOT"] = str(cache_root)
        try:
            require(
                bootstrap.default_machine_cache_root() == cache_root,
                "machine-cache environment override was ignored",
            )
            git_command = bootstrap.git_command("status", "--porcelain")
            require(
                ("core.longpaths=true" in git_command) == (os.name == "nt"),
                "Git long-path policy does not match the host",
            )
            digest = "a" * 32
            source_cache = (
                cache_root
                / "s"
                / digest
                / "d"
            )
            source_cache.mkdir(parents=True)
            require(
                bootstrap.machine_source_cache_is_admitted(source_cache),
                "content-addressed source cache was not admitted",
            )
            require(
                not bootstrap.machine_source_cache_is_admitted(cache_root / "deps-src"),
                "unkeyed source cache was admitted",
            )
            destination = bootstrap.controlled_destination(
                "skia", source_cache, allow_machine_cache=True
            )
            require(
                destination == source_cache / "skia",
                "controlled machine-cache destination changed",
            )

            rejected = False
            try:
                bootstrap.controlled_destination(
                    "skia", cache_root / "outside", allow_machine_cache=True
                )
            except RuntimeError:
                rejected = True
            require(rejected, "arbitrary external dependency cache was admitted")

            readonly_source = cache_root / "readonly-source"
            readonly_source.mkdir()
            readonly_file = readonly_source / "pack.idx"
            readonly_file.write_bytes(b"verified-pack")
            readonly_file.chmod(stat.S_IREAD)
            copied_tree = cache_root / "copied-tree"
            bootstrap.copy_tree_atomic(readonly_source, copied_tree)
            require(
                (copied_tree / "pack.idx").read_bytes() == b"verified-pack",
                "machine-cache copy changed a read-only Git pack",
            )
            require(
                not (os.stat(copied_tree / "pack.idx").st_mode & stat.S_IWUSR),
                "machine-cache copy changed a read-only Git pack mode",
            )
            if os.name == "nt":
                long_source = cache_root / "long-source"
                long_parent = long_source
                while len(str(long_parent)) < 270:
                    long_parent /= "d" * 30
                os.makedirs(bootstrap.filesystem_io_path(long_parent))
                long_file = long_parent / "content.bin"
                with open(bootstrap.filesystem_io_path(long_file), "wb") as stream:
                    stream.write(b"long-path-content")
                long_copy = cache_root / "long-copy"
                bootstrap.copy_tree_atomic(
                    long_source, long_copy, cache_root / "staging"
                )
                copied_long_file = long_copy / long_file.relative_to(long_source)
                with open(
                    bootstrap.filesystem_io_path(copied_long_file), "rb"
                ) as stream:
                    require(
                        stream.read() == b"long-path-content",
                        "machine-cache copy truncated a Windows long path",
                    )
                bootstrap.remove_tree(long_source)
                bootstrap.remove_tree(long_copy)

            escaped = False
            try:
                bootstrap.safe_cache_index_path(cache_root, "../../escape")
            except RuntimeError:
                escaped = True
            require(escaped, "machine-cache index path escaped its root")

            index = bootstrap.empty_machine_cache_index()
            index["skia_profiles"]["test-profile"] = {
                "host": bootstrap.host_key(),
            }
            bootstrap.write_json_atomic(
                bootstrap.machine_cache_index_path(cache_root), index
            )
            require(
                bootstrap.read_machine_cache_index(cache_root) == index,
                "machine-cache index did not round-trip",
            )
            require(
                bootstrap.canonical_sha256({"b": 2, "a": 1})
                == bootstrap.canonical_sha256({"a": 1, "b": 2}),
                "machine-cache identity is not canonical",
            )
            full_digest = "b" * 64
            bootstrap.require_cache_identity(
                full_digest, full_digest, "valid identity rejected"
            )
            identity_rejected = False
            try:
                bootstrap.require_cache_identity(
                    "b" * 32, full_digest, "truncated identity admitted"
                )
            except RuntimeError:
                identity_rejected = True
            require(identity_rejected, "truncated cache identity was admitted")

            receipt_path = cache_root / "receipt.json"
            bootstrap.write_json_atomic(
                receipt_path,
                {
                    "schema_version": bootstrap.MACHINE_CACHE_SCHEMA_VERSION,
                    "kind": "test-cache-receipt",
                },
            )
            require(
                bootstrap.read_cache_receipt(
                    receipt_path, "test-cache-receipt"
                )["kind"]
                == "test-cache-receipt",
                "valid machine-cache receipt was rejected",
            )
            receipt_rejected = False
            try:
                bootstrap.read_cache_receipt(receipt_path, "different-kind")
            except RuntimeError:
                receipt_rejected = True
            require(receipt_rejected, "wrong machine-cache receipt was admitted")
            portable_text = cache_root / "portable.txt"
            portable_text.write_bytes(b"one\ntwo\n")
            crlf_sha = bootstrap.hashlib.sha256(b"one\r\ntwo\r\n").hexdigest()
            require(
                bootstrap.recorded_text_sha256_matches(portable_text, crlf_sha),
                "portable text digest rejected equivalent CRLF metadata",
            )
            unrelated_sha = bootstrap.hashlib.sha256(b"different\n").hexdigest()
            require(
                not bootstrap.recorded_text_sha256_matches(
                    portable_text, unrelated_sha
                ),
                "portable text digest admitted different content",
            )

            malformed = cache_root / "index.json"
            malformed.write_text(
                json.dumps({"schema_version": 999}), encoding="utf-8"
            )
            stale_rejected = False
            try:
                bootstrap.read_machine_cache_index(cache_root)
            except RuntimeError:
                stale_rejected = True
            require(stale_rejected, "stale machine-cache schema was admitted")
            malformed.write_text("[]", encoding="utf-8")
            malformed_rejected = False
            try:
                bootstrap.read_machine_cache_index(cache_root)
            except RuntimeError:
                malformed_rejected = True
            require(malformed_rejected, "non-object machine-cache index was admitted")
        finally:
            if previous is None:
                os.environ.pop("REFUSION_MACHINE_CACHE_ROOT", None)
            else:
                os.environ["REFUSION_MACHINE_CACHE_ROOT"] = previous


if __name__ == "__main__":
    main()
