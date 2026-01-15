from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys


def parse_args(argv: list[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    return ap.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if not args.check:
        print("usage: check_site.py --check", file=sys.stderr)
        return 2

    root = Path(__file__).resolve().parents[1]
    for d in ["docs", "agent", "scripts", "site", "versions"]:
        if not (root / d).is_dir():
            print(f"ERROR: missing directory: {d}/", file=sys.stderr)
            return 1

    versions_path = root / "versions" / "toolchain_versions.json"
    if not versions_path.is_file():
        print("ERROR: missing versions/toolchain_versions.json", file=sys.stderr)
        return 1

    try:
        versions = json.loads(versions_path.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"ERROR: versions/toolchain_versions.json invalid JSON: {e}", file=sys.stderr)
        return 1

    if not isinstance(versions, dict):
        print("ERROR: versions/toolchain_versions.json must be a JSON object", file=sys.stderr)
        return 1
    if versions.get("schema_version") != "x07.website.toolchain-versions@0.1.0":
        print("ERROR: versions/toolchain_versions.json schema_version mismatch", file=sys.stderr)
        return 1
    if versions.get("latest_toolchain_version") is not None and not isinstance(
        versions.get("latest_toolchain_version"), str
    ):
        print(
            "ERROR: versions/toolchain_versions.json latest_toolchain_version must be string|null",
            file=sys.stderr,
        )
        return 1
    if not isinstance(versions.get("versions"), list):
        print(
            "ERROR: versions/toolchain_versions.json versions must be an array",
            file=sys.stderr,
        )
        return 1

    seen_versions: set[str] = set()
    for entry in versions["versions"]:
        if not isinstance(entry, dict):
            print("ERROR: versions entry must be an object", file=sys.stderr)
            return 1
        toolchain_version = entry.get("toolchain_version")
        if not isinstance(toolchain_version, str) or not toolchain_version:
            print(
                "ERROR: versions entry toolchain_version must be non-empty string",
                file=sys.stderr,
            )
            return 1
        if toolchain_version in seen_versions:
            print(f"ERROR: duplicate toolchain_version: {toolchain_version}", file=sys.stderr)
            return 1
        seen_versions.add(toolchain_version)

        docs_dir = root / "docs" / f"v{toolchain_version}"
        agent_dir = root / "agent" / f"v{toolchain_version}"
        if not docs_dir.is_dir():
            print(
                f"ERROR: missing docs for toolchain_version {toolchain_version}: {docs_dir}",
                file=sys.stderr,
            )
            return 1
        if not agent_dir.is_dir():
            print(
                f"ERROR: missing agent for toolchain_version {toolchain_version}: {agent_dir}",
                file=sys.stderr,
            )
            return 1

    latest = versions.get("latest_toolchain_version")
    if latest:
        if latest not in seen_versions:
            print(
                f"ERROR: latest_toolchain_version not in versions: {latest}",
                file=sys.stderr,
            )
            return 1
        for d in ["docs/latest", "agent/latest"]:
            if not (root / d).is_dir():
                print(f"ERROR: missing directory: {d}/", file=sys.stderr)
                return 1
        if not (root / "agent" / "latest" / "index.json").is_file():
            print("ERROR: missing agent/latest/index.json", file=sys.stderr)
            return 1

    print("ok: website repo structure")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
