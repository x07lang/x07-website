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
    for d in ["docs", "agent", "scripts"]:
        if not (root / d).is_dir():
            print(f"ERROR: missing directory: {d}/", file=sys.stderr)
            return 1

    versions_path = root / "versions.json"
    if not versions_path.is_file():
        print("ERROR: missing versions.json", file=sys.stderr)
        return 1

    try:
        versions = json.loads(versions_path.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"ERROR: versions.json invalid JSON: {e}", file=sys.stderr)
        return 1

    if not isinstance(versions, dict):
        print("ERROR: versions.json must be a JSON object", file=sys.stderr)
        return 1
    if versions.get("schema_version") != "x07.website-versions@0.1.0":
        print("ERROR: versions.json schema_version mismatch", file=sys.stderr)
        return 1
    if not isinstance(versions.get("versions"), list):
        print("ERROR: versions.json versions must be an array", file=sys.stderr)
        return 1

    print("ok: website repo structure")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

