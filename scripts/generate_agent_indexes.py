from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path


def parse_args(argv: list[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser()
    ap.add_argument("--agent-dir", required=True, type=Path)
    ap.add_argument("--check", action="store_true")
    return ap.parse_args(argv)


_EXAMPLE_ID_RE = re.compile(r"^(?P<num>\d+)_+(?P<rest>.*)$")


def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _write_text_if_changed(*, path: Path, content: str, check: bool) -> None:
    content = content.replace("\r\n", "\n").replace("\r", "\n")
    if not content.endswith("\n"):
        content += "\n"

    if check:
        if not path.is_file():
            raise SystemExit(f"[CHECK] missing {path}")
        existing = _read_text(path).replace("\r\n", "\n").replace("\r", "\n")
        if existing != content:
            raise SystemExit(f"[CHECK] out of date: {path}")
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _write_json_if_changed(*, path: Path, obj: object, check: bool) -> None:
    content = json.dumps(obj, indent=2)
    _write_text_if_changed(path=path, content=content, check=check)


def _iter_files_sorted(root: Path) -> list[Path]:
    out: list[Path] = []
    for p in root.rglob("*"):
        if not p.is_file():
            continue
        rel = p.relative_to(root)
        if rel.name == ".DS_Store":
            continue
        if any(part.startswith("._") for part in rel.parts):
            continue
        out.append(rel)
    return sorted(out, key=lambda p: p.as_posix())


def _ignore_macos_metadata(_: str, names: list[str]) -> set[str]:
    ignored: set[str] = set()
    for name in names:
        if name == ".DS_Store" or name.startswith("._"):
            ignored.add(name)
    return ignored


def _strip_suffix(name: str, suffix: str) -> str:
    if not name.endswith(suffix):
        raise ValueError(f"expected filename to end with {suffix}: {name}")
    return name[: -len(suffix)]


def _infer_example_purpose(example_id: str) -> str:
    m = _EXAMPLE_ID_RE.match(example_id)
    rest = m.group("rest") if m else example_id
    rest = rest.strip("_")
    if not rest:
        return ""
    return rest.replace("_", " ")


def _parse_skill_summary(skill_md: Path) -> str:
    text = _read_text(skill_md)
    lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    if not lines or lines[0].strip() != "---":
        return ""

    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            frontmatter = lines[1:i]
            break
    else:
        return ""

    for line in frontmatter:
        if line.startswith("description:"):
            return line.removeprefix("description:").strip()
    return ""


def _generate_schemas_index(*, agent_dir: Path, rel_agent_dir: Path, url_prefix: str) -> dict:
    schemas_dir = agent_dir / "schemas"
    items: list[dict] = []
    for p in sorted(schemas_dir.glob("*.schema.json"), key=lambda x: x.name):
        items.append({"id": p.name, "url": f"{url_prefix}/schemas/{p.name}"})
    return {
        "schema_version": "x07.website.agent.schemas_index@v1",
        "generated_from": f"{rel_agent_dir.as_posix()}/schemas/*.schema.json",
        "items": items,
    }


def _generate_examples_index(*, agent_dir: Path, rel_agent_dir: Path, url_prefix: str) -> dict:
    examples_dir = agent_dir / "examples"
    items: list[dict] = []
    for p in sorted(examples_dir.glob("*.x07.json"), key=lambda x: x.name):
        example_id = _strip_suffix(p.name, ".x07.json")
        items.append(
            {
                "id": example_id,
                "purpose": _infer_example_purpose(example_id),
                "url": f"{url_prefix}/examples/{p.name}",
            }
        )
    return {
        "schema_version": "x07.website.agent.examples_index@v1",
        "generated_from": f"{rel_agent_dir.as_posix()}/examples/*.x07.json",
        "items": items,
    }


def _compare_trees(*, a: Path, b: Path, label: str) -> None:
    a_files = _iter_files_sorted(a)
    b_files = _iter_files_sorted(b)
    if a_files != b_files:
        raise SystemExit(f"[CHECK] tree mismatch ({label}): {a} vs {b}")
    for rel in a_files:
        if (a / rel).read_bytes() != (b / rel).read_bytes():
            raise SystemExit(f"[CHECK] file differs ({label}): {rel}")


def _export_skills_pack(*, agent_dir: Path, check: bool) -> list[str]:
    skills_dir = agent_dir / "skills"
    skills_pack = skills_dir / "pack" / "skills"
    if not skills_pack.is_dir():
        return []

    expected_skill_ids: list[str] = []
    for child in sorted(skills_pack.iterdir(), key=lambda p: p.name):
        if not child.is_dir():
            continue
        if not (child / "SKILL.md").is_file():
            continue
        expected_skill_ids.append(child.name)

    exported_skill_ids: list[str] = []
    for child in sorted(skills_dir.iterdir(), key=lambda p: p.name):
        if not child.is_dir():
            continue
        if child.name in ("dev-scripts", "pack"):
            continue
        exported_skill_ids.append(child.name)

    if check:
        if exported_skill_ids != expected_skill_ids:
            raise SystemExit(
                f"[CHECK] exported skills mismatch: expected={expected_skill_ids} actual={exported_skill_ids}"
            )
        for skill_id in expected_skill_ids:
            _compare_trees(
                a=skills_pack / skill_id,
                b=skills_dir / skill_id,
                label=f"skill:{skill_id}",
            )
        return expected_skill_ids

    for skill_id in exported_skill_ids:
        shutil.rmtree(skills_dir / skill_id)

    for skill_id in expected_skill_ids:
        shutil.copytree(
            skills_pack / skill_id,
            skills_dir / skill_id,
            ignore=_ignore_macos_metadata,
        )

    return expected_skill_ids


def _generate_skills_index(
    *, agent_dir: Path, rel_agent_dir: Path, url_prefix: str, skill_ids: list[str]
) -> dict:
    report_schema_by_skill_id: dict[str, str] = {
        "x07-build-run": "x07-host-runner.report.schema.json",
        "x07-os-run": "x07-os-runner.report.schema.json",
        "x07-lint-repair": "x07c.report.schema.json",
        "x07-format": "x07c.report.schema.json",
        "x07-test": "x07test.schema.json",
    }

    items: list[dict] = []
    for skill_id in sorted(skill_ids):
        skill_md = agent_dir / "skills" / skill_id / "SKILL.md"
        report_schema = report_schema_by_skill_id.get(skill_id)
        if report_schema is None:
            report_schema_url = None
        else:
            schema_path = agent_dir / "schemas" / report_schema
            if not schema_path.is_file():
                raise SystemExit(
                    f"missing report schema for skill {skill_id}: {schema_path.relative_to(agent_dir)}"
                )
            report_schema_url = f"{url_prefix}/schemas/{report_schema}"
        items.append(
            {
                "id": skill_id,
                "summary": _parse_skill_summary(skill_md),
                "docs_url": f"{url_prefix}/skills/{skill_id}/SKILL.md",
                "report_schema_url": report_schema_url,
            }
        )
    return {
        "schema_version": "x07.website.agent.skills_index@v1",
        "generated_from": f"{rel_agent_dir.as_posix()}/skills/*/SKILL.md",
        "items": items,
    }


def _generate_skill_descriptor(
    *,
    rel_agent_dir: Path,
    skill_id: str,
    summary: str,
    docs_url: str,
    report_schema_url: str | None,
) -> dict:
    return {
        "schema_version": "x07.website.agent.skill@v1",
        "generated_from": f"{rel_agent_dir.as_posix()}/skills/{skill_id}/SKILL.md",
        "id": skill_id,
        "summary": summary,
        "docs_url": docs_url,
        "report_schema_url": report_schema_url,
    }


def _sync_skill_descriptors(*, agent_dir: Path, rel_agent_dir: Path, skills_index: dict, check: bool) -> None:
    skills_dir = agent_dir / "skills"
    items = skills_index.get("items", [])
    if not isinstance(items, list):
        raise SystemExit(f"invalid skills index items: {skills_dir.relative_to(agent_dir)}/index.json")

    expected_names: set[str] = set()
    for it in items:
        if not isinstance(it, dict):
            raise SystemExit(f"invalid skills index item: {skills_dir.relative_to(agent_dir)}/index.json")
        skill_id = it.get("id")
        summary = it.get("summary")
        docs_url = it.get("docs_url")
        report_schema_url = it.get("report_schema_url")
        if not isinstance(skill_id, str) or not skill_id:
            raise SystemExit(f"invalid skills index item.id: {skills_dir.relative_to(agent_dir)}/index.json")
        if not isinstance(summary, str):
            raise SystemExit(f"invalid skills index item.summary: {skills_dir.relative_to(agent_dir)}/index.json")
        if not isinstance(docs_url, str) or not docs_url:
            raise SystemExit(f"invalid skills index item.docs_url: {skills_dir.relative_to(agent_dir)}/index.json")
        if report_schema_url is not None and not isinstance(report_schema_url, str):
            raise SystemExit(
                f"invalid skills index item.report_schema_url: {skills_dir.relative_to(agent_dir)}/index.json"
            )

        descriptor_name = f"{skill_id}.json"
        expected_names.add(descriptor_name)
        _write_json_if_changed(
            path=skills_dir / descriptor_name,
            obj=_generate_skill_descriptor(
                rel_agent_dir=rel_agent_dir,
                skill_id=skill_id,
                summary=summary,
                docs_url=docs_url,
                report_schema_url=report_schema_url,
            ),
            check=check,
        )

    for p in sorted(skills_dir.glob("*.json"), key=lambda x: x.name):
        if p.name == "index.json":
            continue
        if p.name in expected_names:
            continue
        if check:
            raise SystemExit(f"[CHECK] unexpected skill descriptor: {p.relative_to(agent_dir)}")
        p.unlink()


def _update_agent_index_json(*, agent_dir: Path, check: bool) -> None:
    index_path = agent_dir / "index.json"
    if not index_path.is_file():
        raise SystemExit(f"missing agent index: {index_path}")

    try:
        index = json.loads(index_path.read_text(encoding="utf-8"))
    except Exception as e:
        raise SystemExit(f"invalid JSON: {index_path}: {e}")
    if not isinstance(index, dict):
        raise SystemExit(f"agent index must be a JSON object: {index_path}")

    expected = {
        "schemas_index_url": "schemas/index.json",
        "skills_index_url": "skills/index.json",
        "examples_index_url": "examples/index.json",
    }
    if check:
        for k, v in expected.items():
            if index.get(k) != v:
                raise SystemExit(f"[CHECK] {index_path} {k} must be {v!r}")
        return

    changed = False
    for k, v in expected.items():
        if index.get(k) != v:
            index[k] = v
            changed = True
    if changed:
        index_path.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")


def _generate_site_agent_data_ts(*, repo_root: Path, check: bool) -> None:
    agent_latest = repo_root / "agent" / "latest"
    skills_index = json.loads((agent_latest / "skills" / "index.json").read_text(encoding="utf-8"))
    schemas_index = json.loads(
        (agent_latest / "schemas" / "index.json").read_text(encoding="utf-8")
    )
    examples_index = json.loads(
        (agent_latest / "examples" / "index.json").read_text(encoding="utf-8")
    )
    stdlib_index = json.loads((agent_latest / "stdlib" / "index.json").read_text(encoding="utf-8"))
    stdlib_modules = stdlib_index.get("modules", [])
    module_count = len(stdlib_modules) if isinstance(stdlib_modules, list) else 0

    out = repo_root / "site" / "src" / "pages" / "agent" / "_agent_data.ts"
    content = (
        "/**\n"
        " * THIS FILE IS GENERATED by scripts/generate_agent_indexes.py\n"
        " * DO NOT EDIT BY HAND.\n"
        " */\n\n"
        f"export const latestSkillsIndex = {json.dumps(skills_index, indent=2)} as const;\n\n"
        f"export const latestSchemasIndex = {json.dumps(schemas_index, indent=2)} as const;\n\n"
        f"export const latestExamplesIndex = {json.dumps(examples_index, indent=2)} as const;\n\n"
        "export const latestStdlibIndexMeta = {\n"
        "  url: '/agent/latest/stdlib/index.json',\n"
        f"  module_count: {module_count},\n"
        "} as const;\n"
    )
    _write_text_if_changed(path=out, content=content, check=check)


def _generate_entrypoints_json(*, url_prefix: str) -> dict:
    return {
        "schema_version": "x07.agent.entrypoints@v1",
        "latest": {
            "manifest": f"{url_prefix}/manifest.json",
            "index": f"{url_prefix}/index.json",
            "skills_index": f"{url_prefix}/skills/index.json",
            "schemas_index": f"{url_prefix}/schemas/index.json",
            "examples_index": f"{url_prefix}/examples/index.json",
            "stdlib_index": f"{url_prefix}/stdlib/index.json",
        },
    }


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    repo_root = Path(__file__).resolve().parents[1]

    agent_dir = args.agent_dir
    if not agent_dir.is_absolute():
        agent_dir = (repo_root / agent_dir).resolve()

    try:
        rel_agent_dir = agent_dir.relative_to(repo_root)
    except ValueError:
        print(f"ERROR: agent dir must be within repo root: {agent_dir}", file=sys.stderr)
        return 2

    if not agent_dir.is_dir():
        print(f"ERROR: agent dir not found: {agent_dir}", file=sys.stderr)
        return 2

    url_prefix = "/" + rel_agent_dir.as_posix()

    if not (agent_dir / "schemas").is_dir():
        print(f"ERROR: missing schemas dir: {rel_agent_dir}/schemas", file=sys.stderr)
        return 2
    if not (agent_dir / "skills").is_dir():
        print(f"ERROR: missing skills dir: {rel_agent_dir}/skills", file=sys.stderr)
        return 2
    if not (agent_dir / "examples").is_dir():
        print(f"ERROR: missing examples dir: {rel_agent_dir}/examples", file=sys.stderr)
        return 2

    _update_agent_index_json(agent_dir=agent_dir, check=args.check)

    schemas_index = _generate_schemas_index(
        agent_dir=agent_dir, rel_agent_dir=rel_agent_dir, url_prefix=url_prefix
    )
    _write_json_if_changed(
        path=agent_dir / "schemas" / "index.json",
        obj=schemas_index,
        check=args.check,
    )

    examples_index = _generate_examples_index(
        agent_dir=agent_dir, rel_agent_dir=rel_agent_dir, url_prefix=url_prefix
    )
    _write_json_if_changed(
        path=agent_dir / "examples" / "index.json",
        obj=examples_index,
        check=args.check,
    )

    skill_ids = _export_skills_pack(agent_dir=agent_dir, check=args.check)
    skills_index = _generate_skills_index(
        agent_dir=agent_dir,
        rel_agent_dir=rel_agent_dir,
        url_prefix=url_prefix,
        skill_ids=skill_ids,
    )
    _write_json_if_changed(
        path=agent_dir / "skills" / "index.json",
        obj=skills_index,
        check=args.check,
    )
    _sync_skill_descriptors(
        agent_dir=agent_dir,
        rel_agent_dir=rel_agent_dir,
        skills_index=skills_index,
        check=args.check,
    )

    if rel_agent_dir.as_posix() == "agent/latest":
        entrypoints = _generate_entrypoints_json(url_prefix=url_prefix)
        _write_json_if_changed(
            path=agent_dir / "entrypoints.json",
            obj=entrypoints,
            check=args.check,
        )
        _generate_site_agent_data_ts(repo_root=repo_root, check=args.check)

    print(f"ok: generated agent indexes for {rel_agent_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
