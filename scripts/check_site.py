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

    def iter_files_sorted(dir_path: Path) -> list[Path]:
        out: list[Path] = []
        for p in dir_path.rglob("*"):
            if not p.is_file():
                continue
            rel = p.relative_to(dir_path)
            if rel.name == ".DS_Store":
                continue
            if any(part.startswith("._") for part in rel.parts):
                continue
            out.append(rel)
        return sorted(out, key=lambda p: p.as_posix())

    def compare_trees(*, a: Path, b: Path, label: str) -> int:
        a_files = iter_files_sorted(a)
        b_files = iter_files_sorted(b)
        if a_files != b_files:
            return err(f"tree mismatch ({label}): {a} vs {b}")
        for rel in a_files:
            if (a / rel).read_bytes() != (b / rel).read_bytes():
                return err(f"file differs ({label}): {rel}")
        return 0

    def compare_agent_payload(*, latest_dir: Path, version_dir: Path) -> int:
        # agent/latest and agent/vX.Y.Z differ in index metadata because they embed URL prefixes.
        # Compare the underlying published payload to prevent accidental manual edits.

        def iter_payload_files(agent_dir: Path) -> list[Path]:
            out: list[Path] = []
            for p in agent_dir.rglob("*"):
                if not p.is_file():
                    continue
                rel = p.relative_to(agent_dir)
                if rel.name == ".DS_Store":
                    continue
                if any(part.startswith("._") for part in rel.parts):
                    continue

                # Exclude index metadata files that include URL prefixes or generated_from paths.
                if rel == Path("index.json"):
                    continue
                if rel == Path("entrypoints.json"):
                    continue
                if rel == Path("manifest.json"):
                    continue
                if rel == Path("schemas/index.json"):
                    continue
                if rel == Path("examples/index.json"):
                    continue
                if rel == Path("examples/catalog.json"):
                    continue
                if rel == Path("packages/index.json"):
                    continue
                if rel == Path("catalog/index.json"):
                    continue
                if rel == Path("skills/index.json"):
                    continue
                if rel.parent == Path("skills") and rel.suffix == ".json":
                    # skills/<skill_id>.json descriptors embed docs_url/report_schema_url.
                    continue

                out.append(rel)
            return sorted(out, key=lambda p: p.as_posix())

        latest_files = iter_payload_files(latest_dir)
        version_files = iter_payload_files(version_dir)
        if latest_files != version_files:
            return err(f"tree mismatch (agent/latest payload): {latest_dir} vs {version_dir}")
        for rel in latest_files:
            if (latest_dir / rel).read_bytes() != (version_dir / rel).read_bytes():
                return err(f"file differs (agent/latest payload): {rel}")
        return 0

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

        def opt_str_field(name: str) -> str | None:
            v = index.get(name)
            if v is None:
                return None
            if not isinstance(v, str) or not v:
                print(
                    f"ERROR: {index_path.relative_to(root)} {name} must be non-empty string when present",
                    file=sys.stderr,
                )
                return ""
            return v

        manifest_url = req_str_field("manifest_url")
        schemas_dir = req_str_field("schemas_dir")
        skills_dir = req_str_field("skills_dir")
        stdlib_index_url = req_str_field("stdlib_index_url")
        examples_dir = req_str_field("examples_dir")
        packages_dir = opt_str_field("packages_dir")
        schemas_index_url = req_str_field("schemas_index_url")
        skills_index_url = req_str_field("skills_index_url")
        examples_index_url = req_str_field("examples_index_url")
        examples_catalog_index_url = opt_str_field("examples_catalog_index_url")
        packages_index_url = opt_str_field("packages_index_url")
        catalog_index_url = opt_str_field("catalog_index_url")
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
        if "" in (packages_dir, examples_catalog_index_url, packages_index_url, catalog_index_url):
            return 1

        if not (agent_dir / manifest_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{manifest_url}")
        if not (agent_dir / schemas_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{schemas_dir}")
        if not (agent_dir / skills_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{skills_dir}")
        if not (agent_dir / examples_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{examples_dir}")
        if packages_dir and not (agent_dir / packages_dir).is_dir():
            return err(f"missing {agent_dir.relative_to(root)}/{packages_dir}")
        if not (agent_dir / stdlib_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{stdlib_index_url}")
        if not (agent_dir / schemas_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{schemas_index_url}")
        if not (agent_dir / skills_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{skills_index_url}")
        if not (agent_dir / examples_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{examples_index_url}")
        if examples_catalog_index_url and not (agent_dir / examples_catalog_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{examples_catalog_index_url}")
        if packages_index_url and not (agent_dir / packages_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{packages_index_url}")
        if catalog_index_url and not (agent_dir / catalog_index_url).is_file():
            return err(f"missing {agent_dir.relative_to(root)}/{catalog_index_url}")

        hidden_dirs = []
        for hidden_dir in (".agent", ".codex"):
            hidden_dirs.extend(agent_dir.rglob(hidden_dir))
        if hidden_dirs:
            rel = hidden_dirs[0].relative_to(root)
            return err(f"hidden directory {hidden_dirs[0].name} must not be published: {rel}")

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

        # ---- examples catalog index (optional) ----
        if examples_catalog_index_url:
            rc, examples_catalog_index = check_index_common(
                index_path=agent_dir / examples_catalog_index_url,
                expected_schema_version="x07.website.agent.examples_catalog_index@v1",
                required_item_keys=["id", "url", "path"],
            )
            if rc != 0 or examples_catalog_index is None:
                return 1

            if agent_rel == "agent/latest":
                docs_examples_root = root / "docs" / "latest" / "examples"
            elif agent_rel.startswith("agent/v"):
                version = agent_rel.removeprefix("agent/v")
                docs_examples_root = root / "docs" / f"v{version}" / "examples"
            else:
                return err(f"unsupported agent dir for examples catalog: {agent_rel}")

            if not docs_examples_root.is_dir():
                return err(f"missing docs examples root for catalog: {docs_examples_root.relative_to(root)}")
            catalog_files_root = agent_dir / "examples" / "catalog-files"
            if not catalog_files_root.is_dir():
                return err(f"missing catalog files dir: {catalog_files_root.relative_to(root)}")

            cat_items = examples_catalog_index.get("items", [])
            cat_ids: list[str] = []
            for it in cat_items:
                eid = it.get("id")
                url = it.get("url")
                rel_path = it.get("path")
                purpose = it.get("purpose")
                scope = it.get("scope")
                if not isinstance(eid, str) or not eid:
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} item.id must be non-empty string"
                    )
                if not isinstance(url, str) or not url:
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} item.url must be non-empty string"
                    )
                if not isinstance(rel_path, str) or not rel_path:
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} item.path must be non-empty string"
                    )
                if purpose is not None and not isinstance(purpose, str):
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} item.purpose must be string|null"
                    )
                if scope is not None and scope not in ("top_level", "nested"):
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} item.scope must be top_level|nested|null"
                    )
                if not rel_path.endswith(".x07.json"):
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} item.path must end with .x07.json"
                    )
                expected_url = f"{agent_url_prefix}/examples/catalog-files/{rel_path}"
                if url != expected_url:
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} item.url mismatch: expected {expected_url}"
                    )
                target = catalog_files_root / rel_path
                if not target.is_file():
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} missing catalog file: {target.relative_to(root)}"
                    )
                source = docs_examples_root / rel_path
                if not source.is_file():
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} missing source docs example file: {source.relative_to(root)}"
                    )
                if source.read_bytes() != target.read_bytes():
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} catalog file differs from docs source: {target.relative_to(root)}"
                    )
                expected_scope = "top_level" if len(Path(rel_path).parts) == 1 else "nested"
                if scope is not None and scope != expected_scope:
                    return err(
                        f"{(agent_dir / examples_catalog_index_url).relative_to(root)} item.scope mismatch for {rel_path}"
                    )
                cat_ids.append(eid)

            if cat_ids != sorted(cat_ids):
                return err(f"{(agent_dir / examples_catalog_index_url).relative_to(root)} items must be sorted by id")

            expected_cat_ids = sorted(
                p.relative_to(docs_examples_root).as_posix().removesuffix(".x07.json")
                for p in docs_examples_root.rglob("*.x07.json")
            )
            if cat_ids != expected_cat_ids:
                return err(
                    f"{(agent_dir / examples_catalog_index_url).relative_to(root)} items do not match docs examples"
                )
            actual_catalog_paths = sorted(
                p.relative_to(catalog_files_root).as_posix().removesuffix(".x07.json")
                for p in catalog_files_root.rglob("*.x07.json")
            )
            if actual_catalog_paths != expected_cat_ids:
                return err(
                    f"{(agent_dir / examples_catalog_index_url).relative_to(root)} catalog-files tree does not match docs examples"
                )

        # ---- packages index (optional for older toolchains) ----
        if packages_index_url:
            rc, packages_index = check_index_common(
                index_path=agent_dir / packages_index_url,
                expected_schema_version="x07.website.agent.packages_index@v1",
                required_item_keys=["name", "version", "url"],
            )
            if rc != 0 or packages_index is None:
                return 1

            def parse_semver(v: str) -> tuple[int, int, int] | None:
                parts = v.split(".")
                if len(parts) != 3:
                    return None
                try:
                    major = int(parts[0])
                    minor = int(parts[1])
                    patch = int(parts[2])
                except ValueError:
                    return None
                if major < 0 or minor < 0 or patch < 0:
                    return None
                return (major, minor, patch)

            pkg_items = packages_index.get("items", [])
            pkg_tuples: list[tuple[str, str]] = []
            for it in pkg_items:
                name = it.get("name")
                version = it.get("version")
                url = it.get("url")
                if not isinstance(name, str) or not name:
                    return err(
                        f"{(agent_dir / packages_index_url).relative_to(root)} item.name must be non-empty string"
                    )
                if not isinstance(version, str) or not version:
                    return err(
                        f"{(agent_dir / packages_index_url).relative_to(root)} item.version must be non-empty string"
                    )
                if not isinstance(url, str) or not url:
                    return err(
                        f"{(agent_dir / packages_index_url).relative_to(root)} item.url must be non-empty string"
                    )
                if not url.startswith(f"{agent_url_prefix}/packages/"):
                    return err(
                        f"{(agent_dir / packages_index_url).relative_to(root)} item.url must start with {agent_url_prefix}/packages/"
                    )
                if not (root / url.lstrip("/")).is_file():
                    return err(
                        f"{(agent_dir / packages_index_url).relative_to(root)} missing file for url: {url}"
                    )
                pkg_tuples.append((name, version))

            expected_sorted = sorted(
                pkg_tuples, key=lambda t: (t[0], parse_semver(t[1]) or (0, 0, 0), t[1])
            )
            if pkg_tuples != expected_sorted:
                return err(f"{(agent_dir / packages_index_url).relative_to(root)} items must be sorted")

            packages_root = agent_dir / (packages_dir or "packages")
            expected: list[tuple[str, str]] = []
            if packages_root.is_dir():
                for pkg_dir in sorted(packages_root.iterdir(), key=lambda p: p.name):
                    if not pkg_dir.is_dir():
                        continue
                    for ver_dir in sorted(pkg_dir.iterdir(), key=lambda p: p.name):
                        if not ver_dir.is_dir():
                            continue
                        if not (ver_dir / "index.json").is_file():
                            continue
                        expected.append((pkg_dir.name, ver_dir.name))
            expected = sorted(expected, key=lambda t: (t[0], parse_semver(t[1]) or (0, 0, 0), t[1]))
            if pkg_tuples != expected:
                return err(f"{(agent_dir / packages_index_url).relative_to(root)} items do not match packages dir")

        # ---- catalog index (optional for older toolchains) ----
        if catalog_index_url:
            rc, catalog_index = check_index_common(
                index_path=agent_dir / catalog_index_url,
                expected_schema_version="x07.website.agent.catalog_index@v1",
                required_item_keys=["id", "url"],
            )
            if rc != 0 or catalog_index is None:
                return 1

            cat_items = catalog_index.get("items", [])
            cat_ids: list[str] = []
            for it in cat_items:
                cid = it.get("id")
                url = it.get("url")
                if not isinstance(cid, str) or not cid:
                    return err(
                        f"{(agent_dir / catalog_index_url).relative_to(root)} item.id must be non-empty string"
                    )
                if not isinstance(url, str) or not url:
                    return err(
                        f"{(agent_dir / catalog_index_url).relative_to(root)} item.url must be non-empty string"
                    )
                if not url.startswith(f"{agent_url_prefix}/catalog/"):
                    return err(
                        f"{(agent_dir / catalog_index_url).relative_to(root)} item.url must start with {agent_url_prefix}/catalog/"
                    )
                if not (root / url.lstrip("/")).is_file():
                    return err(f"{(agent_dir / catalog_index_url).relative_to(root)} missing file for url: {url}")
                cat_ids.append(cid)

            if cat_ids != sorted(cat_ids):
                return err(f"{(agent_dir / catalog_index_url).relative_to(root)} items must be sorted by id")

            expected_cat_ids = sorted(
                p.name.removesuffix(".json")
                for p in (agent_dir / "catalog").glob("*.json")
                if p.name != "index.json"
            )
            if cat_ids != expected_cat_ids:
                return err(f"{(agent_dir / catalog_index_url).relative_to(root)} items do not match catalog dir")

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
            if any(x in docs_url for x in ("/.agent/", "/.codex/")):
                return err(
                    f"{(agent_dir / skills_index_url).relative_to(root)} item.docs_url must not contain hidden paths: {docs_url}"
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

        skills_root = agent_dir / skills_dir
        expected_skill_json = {"index.json"} | {f"{sid}.json" for sid in skill_ids}
        actual_skill_json = sorted(p.name for p in skills_root.glob("*.json"))
        if actual_skill_json != sorted(expected_skill_json):
            return err(f"{skills_root.relative_to(root)} JSON files mismatch")

        for it in skill_items:
            sid = it.get("id")
            docs_url = it.get("docs_url")
            report_schema_url = it.get("report_schema_url")
            if not isinstance(sid, str) or not sid:
                return err(f"{(agent_dir / skills_index_url).relative_to(root)} item.id must be non-empty string")

            descriptor_path = skills_root / f"{sid}.json"
            if not descriptor_path.is_file():
                return err(f"missing {descriptor_path.relative_to(root)}")
            try:
                descriptor = read_json(descriptor_path)
            except ValueError as e:
                return err(str(e))
            if not isinstance(descriptor, dict):
                return err(f"{descriptor_path.relative_to(root)} must be a JSON object")
            if descriptor.get("schema_version") != "x07.website.agent.skill@v1":
                return err(f"{descriptor_path.relative_to(root)} schema_version mismatch")
            if descriptor.get("id") != sid:
                return err(f"{descriptor_path.relative_to(root)} id mismatch")
            if descriptor.get("docs_url") != docs_url:
                return err(f"{descriptor_path.relative_to(root)} docs_url mismatch")
            if descriptor.get("report_schema_url") != report_schema_url:
                return err(f"{descriptor_path.relative_to(root)} report_schema_url mismatch")
            if not isinstance(descriptor.get("summary"), str):
                return err(f"{descriptor_path.relative_to(root)} summary must be a string")

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
                "examples_catalog_index": "/agent/latest/examples/catalog.json",
                "packages_index": "/agent/latest/packages/index.json",
                "stdlib_index": "/agent/latest/stdlib/index.json",
                "catalog_index": "/agent/latest/catalog/index.json",
                "capabilities": "/agent/latest/catalog/capabilities.json",
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
        rc = compare_trees(
            a=root / "docs" / "latest",
            b=root / "docs" / f"v{latest}",
            label="docs/latest",
        )
        if rc != 0:
            return rc
        rc = compare_agent_payload(
            latest_dir=root / "agent" / "latest",
            version_dir=root / "agent" / f"v{latest}",
        )
        if rc != 0:
            return rc
        rc = check_agent_dir(root / "agent" / "latest")
        if rc != 0:
            return rc

    print("ok: website repo structure")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
