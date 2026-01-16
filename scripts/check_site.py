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

    def err(msg: str) -> int:
        print(f"ERROR: {msg}", file=sys.stderr)
        return 1

    def read_json(path: Path) -> dict | list | str | int | float | bool | None:
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except Exception as e:
            raise ValueError(f"{path} invalid JSON: {e}")

    def check_agent_dir(agent_dir: Path) -> int:
        index_path = agent_dir / "index.json"
        if not index_path.is_file():
            return err(f"missing {index_path.relative_to(root)}")

        try:
            index = read_json(index_path)
        except ValueError as e:
            return err(str(e))

        if not isinstance(index, dict):
            return err(f"{index_path.relative_to(root)} must be a JSON object")

        def req_str_field(name: str) -> str | None:
            v = index.get(name)
            if not isinstance(v, str) or not v:
                print(
                    f"ERROR: {index_path.relative_to(root)} {name} must be non-empty string",
                    file=sys.stderr,
                )
                return None
            return v

        manifest_url = req_str_field("manifest_url")
        schemas_dir = req_str_field("schemas_dir")
        skills_dir = req_str_field("skills_dir")
        stdlib_index_url = req_str_field("stdlib_index_url")
        if None in (manifest_url, schemas_dir, skills_dir, stdlib_index_url):
            return 1

        if not (agent_dir / manifest_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{manifest_url}")
        if not (agent_dir / schemas_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{schemas_dir}")
        if not (agent_dir / skills_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{skills_dir}")
        if not (agent_dir / stdlib_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{stdlib_index_url}")

        schema_files = list((agent_dir / schemas_dir).glob("*.schema.json"))
        if not schema_files:
            return err(f"no schemas found under {agent_dir.relative_to(root)}/{schemas_dir}")

        return 0

    for d in ["docs", "agent", "scripts", "site", "versions"]:
        if not (root / d).is_dir():
            return err(f"missing directory: {d}/")

    versions_path = root / "versions" / "toolchain_versions.json"
    if not versions_path.is_file():
        return err("missing versions/toolchain_versions.json")

    try:
        versions = read_json(versions_path)
    except ValueError as e:
        return err(str(e))

    if not isinstance(versions, dict):
        return err("versions/toolchain_versions.json must be a JSON object")
    if versions.get("schema_version") != "x07.website.toolchain-versions@0.1.0":
        return err("versions/toolchain_versions.json schema_version mismatch")
    if versions.get("latest_toolchain_version") is not None and not isinstance(
        versions.get("latest_toolchain_version"), str
    ):
        return err("versions/toolchain_versions.json latest_toolchain_version must be string|null")
    if not isinstance(versions.get("versions"), list):
        return err("versions/toolchain_versions.json versions must be an array")

    seen_versions: set[str] = set()
    for entry in versions["versions"]:
        if not isinstance(entry, dict):
            return err("versions entry must be an object")
        toolchain_version = entry.get("toolchain_version")
        if not isinstance(toolchain_version, str) or not toolchain_version:
            return err("versions entry toolchain_version must be non-empty string")
        if toolchain_version in seen_versions:
            return err(f"duplicate toolchain_version: {toolchain_version}")
        seen_versions.add(toolchain_version)

        docs_dir = root / "docs" / f"v{toolchain_version}"
        agent_dir = root / "agent" / f"v{toolchain_version}"
        if not docs_dir.is_dir():
            return err(f"missing docs for toolchain_version {toolchain_version}: {docs_dir}")
        if not agent_dir.is_dir():
            return err(f"missing agent for toolchain_version {toolchain_version}: {agent_dir}")

        rc = check_agent_dir(agent_dir)
        if rc != 0:
            return rc

    latest = versions.get("latest_toolchain_version")
    if latest:
        if latest not in seen_versions:
            return err(f"latest_toolchain_version not in versions: {latest}")
        for d in ["docs/latest", "agent/latest"]:
            if not (root / d).is_dir():
                return err(f"missing directory: {d}/")
        rc = check_agent_dir(root / "agent" / "latest")
        if rc != 0:
            return rc

    print("ok: website repo structure")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
