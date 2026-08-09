#!/usr/bin/env python3
"""Verify immutable Desktop video-import v1 fixture and oracle receipts."""

from __future__ import annotations

import hashlib
import json
import pathlib
import sys


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def fail(message: str) -> None:
    raise ValueError(message)


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    contract_path = root / "contracts/media/video-import-v1-fixtures.json"
    contract = json.loads(contract_path.read_text())
    if contract.get("status") != "accepted-materialized":
        fail(f"unexpected corpus status: {contract.get('status')}")

    for row in contract["rows"]:
        payload = root / row["payload"]
        oracle = root / row["oracle"]
        fixture_path = payload.parent / "fixture.json"
        for path in (payload, oracle, fixture_path):
            if not path.is_file():
                fail(f"missing fixture artifact: {path.relative_to(root)}")

        fixture = json.loads(fixture_path.read_text())
        if fixture["id"] != row["id"]:
            fail(f"fixture id mismatch for {row['id']}")
        if fixture["expected"] != row["expected"]:
            fail(f"expected-result mismatch for {row['id']}")
        if fixture["payload"] != payload.name:
            fail(f"payload name mismatch for {row['id']}")

        actual_payload_digest = sha256(payload)
        actual_oracle_digest = sha256(oracle)
        if actual_payload_digest != row["sha256"] or actual_payload_digest != fixture["sha256"]:
            fail(f"payload digest mismatch for {row['id']}")
        if payload.stat().st_size != row["byte_size"] or payload.stat().st_size != fixture["byte_size"]:
            fail(f"payload byte-size mismatch for {row['id']}")
        if actual_oracle_digest != row["oracle_sha256"] or actual_oracle_digest != fixture["oracle_sha256"]:
            fail(f"oracle digest mismatch for {row['id']}")

    print(f"video-import-v1 fixtures: {len(contract['rows'])} rows, all receipts verified")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, OSError, ValueError, json.JSONDecodeError) as error:
        print(f"video-import-v1 fixture verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
