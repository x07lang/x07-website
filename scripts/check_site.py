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
        examples_dir = req_str_field("examples_dir")
        schemas_index_url = req_str_field("schemas_index_url")
        skills_index_url = req_str_field("skills_index_url")
        examples_index_url = req_str_field("examples_index_url")
        if None in (
            manifest_url,
            schemas_dir,
            skills_dir,
            stdlib_index_url,
            examples_dir,
            schemas_index_url,
            skills_index_url,
            examples_index_url,
        ):
            return 1

        if not (agent_dir / manifest_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{manifest_url}")
        if not (agent_dir / schemas_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{schemas_dir}")
        if not (agent_dir / skills_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{skills_dir}")
        if not (agent_dir / examples_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{examples_dir}")
        if not (agent_dir / stdlib_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{stdlib_index_url}")
        if not (agent_dir / schemas_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{schemas_index_url}")
        if not (agent_dir / skills_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{skills_index_url}")
        if not (agent_dir / examples_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{examples_index_url}")

        codex_dirs = list(agent_dir.rglob(".codex"))
        if codex_dirs:
            rel = codex_dirs[0].relative_to(root)
            return err(f"hidden directory .codex must not be published: {rel}")

        schema_files = list((agent_dir / schemas_dir).glob("*.schema.json"))
        if not schema_files:
            return err(f"no schemas found under {agent_dir.relative_to(root)}/{schemas_dir}")

        agent_rel = agent_dir.relative_to(root).as_posix()
        agent_url_prefix = f"/{agent_rel}"

        def check_index_common(
            *, index_path: Path, expected_schema_version: str, required_item_keys: list[str]
        ) -> tuple[int, dict] | tuple[int, None]:
            try:
                doc = read_json(index_path)
            except ValueError as e:
                return err(str(e)), None
            if not isinstance(doc, dict):
                return err(f"{index_path.relative_to(root)} must be a JSON object"), None
            if doc.get("schema_version") != expected_schema_version:
                return err(f"{index_path.relative_to(root)} schema_version mismatch"), None
            items = doc.get("items")
            if not isinstance(items, list):
                return err(f"{index_path.relative_to(root)} items must be an array"), None
            for it in items:
                if not isinstance(it, dict):
                    return err(f"{index_path.relative_to(root)} items must be objects"), None
                for k in required_item_keys:
                    if k not in it:
                        return err(f"{index_path.relative_to(root)} items[*] missing key: {k}"), None
            return 0, doc

        # ---- schemas index ----
        rc, schemas_index = check_index_common(
            index_path=agent_dir / schemas_index_url,
            expected_schema_version="x07.website.agent.schemas_index@v1",
            required_item_keys=["id", "url"],
        )
        if rc != 0 or schemas_index is None:
            return 1
        schema_items = schemas_index.get("items", [])
        schema_ids: list[str] = []
        for it in schema_items:
            sid = it.get("id")
            url = it.get("url")
            if not isinstance(sid, str) or not sid:
                return err(f"{(agent_dir / schemas_index_url).relative_to(root)} item.id must be non-empty string")
            if not isinstance(url, str) or not url:
                return err(f"{(agent_dir / schemas_index_url).relative_to(root)} item.url must be non-empty string")
            if not url.startswith(f"{agent_url_prefix}/schemas/"):
                return err(
                    f"{(agent_dir / schemas_index_url).relative_to(root)} item.url must start with {agent_url_prefix}/schemas/"
                )
            if not (root / url.lstrip("/")).is_file():
                return err(f"{(agent_dir / schemas_index_url).relative_to(root)} missing file for url: {url}")
            schema_ids.append(sid)
        if schema_ids != sorted(schema_ids):
            return err(f"{(agent_dir / schemas_index_url).relative_to(root)} items must be sorted by id")
        expected_schema_ids = sorted(p.name for p in (agent_dir / schemas_dir).glob("*.schema.json"))
        if schema_ids != expected_schema_ids:
            return err(f"{(agent_dir / schemas_index_url).relative_to(root)} items do not match schemas_dir")

        # ---- examples index ----
        rc, examples_index = check_index_common(
            index_path=agent_dir / examples_index_url,
            expected_schema_version="x07.website.agent.examples_index@v1",
            required_item_keys=["id", "url"],
        )
        if rc != 0 or examples_index is None:
            return 1
        example_items = examples_index.get("items", [])
        example_ids: list[str] = []
        for it in example_items:
            eid = it.get("id")
            url = it.get("url")
            purpose = it.get("purpose")
            if not isinstance(eid, str) or not eid:
                return err(f"{(agent_dir / examples_index_url).relative_to(root)} item.id must be non-empty string")
            if purpose is not None and not isinstance(purpose, str):
                return err(f"{(agent_dir / examples_index_url).relative_to(root)} item.purpose must be string|null")
            if not isinstance(url, str) or not url:
                return err(f"{(agent_dir / examples_index_url).relative_to(root)} item.url must be non-empty string")
            if not url.startswith(f"{agent_url_prefix}/examples/"):
                return err(
                    f"{(agent_dir / examples_index_url).relative_to(root)} item.url must start with {agent_url_prefix}/examples/"
                )
            if not (root / url.lstrip("/")).is_file():
                return err(f"{(agent_dir / examples_index_url).relative_to(root)} missing file for url: {url}")
            example_ids.append(eid)
        if example_ids != sorted(example_ids):
            return err(f"{(agent_dir / examples_index_url).relative_to(root)} items must be sorted by id")
        expected_example_ids = sorted(
            p.name.removesuffix(".x07.json")
            for p in (agent_dir / examples_dir).glob("*.x07.json")
        )
        if example_ids != expected_example_ids:
            return err(f"{(agent_dir / examples_index_url).relative_to(root)} items do not match examples_dir")

        # ---- skills index ----
        rc, skills_index = check_index_common(
            index_path=agent_dir / skills_index_url,
            expected_schema_version="x07.website.agent.skills_index@v1",
            required_item_keys=["id", "docs_url", "report_schema_url"],
        )
        if rc != 0 or skills_index is None:
            return 1
        skill_items = skills_index.get("items", [])
        skill_ids: list[str] = []
        for it in skill_items:
            sid = it.get("id")
            docs_url = it.get("docs_url")
            report_schema_url = it.get("report_schema_url")
            if not isinstance(sid, str) or not sid:
                return err(f"{(agent_dir / skills_index_url).relative_to(root)} item.id must be non-empty string")
            if not isinstance(docs_url, str) or not docs_url:
                return err(f"{(agent_dir / skills_index_url).relative_to(root)} item.docs_url must be non-empty string")
            if "/.codex/" in docs_url:
                return err(
                    f"{(agent_dir / skills_index_url).relative_to(root)} item.docs_url must not contain /.codex/: {docs_url}"
                )
            if not docs_url.startswith(f"{agent_url_prefix}/skills/"):
                return err(
                    f"{(agent_dir / skills_index_url).relative_to(root)} item.docs_url must start with {agent_url_prefix}/skills/"
                )
            if not (root / docs_url.lstrip("/")).is_file():
                return err(f"{(agent_dir / skills_index_url).relative_to(root)} missing file for docs_url: {docs_url}")
            if report_schema_url is not None:
                if not isinstance(report_schema_url, str) or not report_schema_url:
                    return err(
                        f"{(agent_dir / skills_index_url).relative_to(root)} item.report_schema_url must be string|null"
                    )
                if not report_schema_url.startswith(f"{agent_url_prefix}/schemas/"):
                    return err(
                        f"{(agent_dir / skills_index_url).relative_to(root)} item.report_schema_url must start with {agent_url_prefix}/schemas/"
                    )
                if not (root / report_schema_url.lstrip("/")).is_file():
                    return err(
                        f"{(agent_dir / skills_index_url).relative_to(root)} missing file for report_schema_url: {report_schema_url}"
                    )
            skill_ids.append(sid)
        if skill_ids != sorted(skill_ids):
            return err(f"{(agent_dir / skills_index_url).relative_to(root)} items must be sorted by id")

        # ---- entrypoints (latest only) ----
        if agent_rel == "agent/latest":
            entrypoints_path = agent_dir / "entrypoints.json"
            if not entrypoints_path.is_file():
                return err(f"missing {entrypoints_path.relative_to(root)}")
            try:
                entrypoints = read_json(entrypoints_path)
            except ValueError as e:
                return err(str(e))
            if not isinstance(entrypoints, dict):
                return err(f"{entrypoints_path.relative_to(root)} must be a JSON object")
            if entrypoints.get("schema_version") != "x07.agent.entrypoints@v1":
                return err(f"{entrypoints_path.relative_to(root)} schema_version mismatch")
            latest_obj = entrypoints.get("latest")
            if not isinstance(latest_obj, dict):
                return err(f"{entrypoints_path.relative_to(root)} latest must be an object")
            expected_latest = {
                "manifest": "/agent/latest/manifest.json",
                "index": "/agent/latest/index.json",
                "skills_index": "/agent/latest/skills/index.json",
                "schemas_index": "/agent/latest/schemas/index.json",
                "examples_index": "/agent/latest/examples/index.json",
                "stdlib_index": "/agent/latest/stdlib/index.json",
            }
            for k, v in expected_latest.items():
                if latest_obj.get(k) != v:
                    return err(f"{entrypoints_path.relative_to(root)} latest.{k} must be {v!r}")
                if not (root / v.lstrip("/")).is_file():
                    return err(f"{entrypoints_path.relative_to(root)} missing file for latest.{k}: {v}")

        return 0

    for d in ["docs", "agent", "scripts", "site", "versions"]:
        if not (root / d).is_dir():
            return err(f"missing directory: {d}/")

    agent_contract = root / "docs" / "latest" / "site" / "agent-portal-contract.md"
    if not agent_contract.is_file():
        return err(f"missing {agent_contract.relative_to(root)}")

    versions_path = root / "versions" / "toolchain_versions.json"
    if not versions_path.is_file():
        return err("missing versions/toolchain_versions.json")

    static_versions_path = root / "site" / "static" / "versions" / "toolchain_versions.json"
    if not static_versions_path.is_file():
        return err("missing site/static/versions/toolchain_versions.json")

    try:
        versions = read_json(versions_path)
    except ValueError as e:
        return err(str(e))

    try:
        static_versions = read_json(static_versions_path)
    except ValueError as e:
        return err(str(e))

    if not isinstance(versions, dict):
        return err("versions/toolchain_versions.json must be a JSON object")
    if versions != static_versions:
        return err("site/static/versions/toolchain_versions.json must match versions/toolchain_versions.json")
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
