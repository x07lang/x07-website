from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from pathlib import PurePosixPath
import shutil
import tarfile
import tempfile
import sys


def parse_args(argv: list[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser()
    ap.add_argument("--toolchain-version", required=True)
    ap.add_argument("--bundle", required=True, type=Path)
    ap.add_argument(
        "--toolchain-repo",
        type=Path,
        default=None,
        help="Defaults to sibling repo ../x07",
    )
    ap.add_argument("--published-at-utc", default=None)
    ap.add_argument("--set-latest", action="store_true")
    return ap.parse_args(argv)


def _sha256_path(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _parse_semver(v: str) -> tuple[int, int, int] | None:
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


def _sort_toolchain_versions_desc(versions: list[dict]) -> list[dict]:
    def sort_key(e: dict) -> tuple[int, int, int, str]:
        v = e.get("toolchain_version")
        if not isinstance(v, str):
            return (-1, -1, -1, "")
        parsed = _parse_semver(v)
        if parsed is None:
            return (-1, -1, -1, v)
        return (*parsed, v)

    return sorted(versions, key=sort_key, reverse=True)


def _safe_member_path(member_name: str) -> PurePosixPath | None:
    name = member_name.lstrip("/")
    if name in ("", "."):
        return None
    path = PurePosixPath(name)
    if ".." in path.parts:
        raise ValueError(f"unsafe tar member path: {member_name}")
    if path.is_absolute():
        raise ValueError(f"unsafe tar member absolute path: {member_name}")
    return path


def _extract_human_docs_bundle(bundle_path: Path, out_dir: Path) -> list[dict]:
    out_dir.mkdir(parents=True, exist_ok=True)

    extracted: list[dict] = []
    with tarfile.open(bundle_path, mode="r:*") as tf:
        for member in tf.getmembers():
            member_path = _safe_member_path(member.name)
            if member_path is None:
                continue
            if member.isdir():
                continue
            if not member.isreg():
                raise ValueError(f"unsupported tar entry type: {member.name}")

            if member_path.parts[0] != "docs":
                continue
            rel = PurePosixPath(*member_path.parts[1:])
            if rel == PurePosixPath():
                continue

            # Drop macOS metadata (AppleDouble / Finder files) from bundles to keep
            # sync outputs deterministic and avoid polluting the docs tree.
            if any(part.startswith("._") for part in rel.parts):
                continue
            if rel.name == ".DS_Store":
                continue

            out_path = out_dir / Path(str(rel))
            out_path.parent.mkdir(parents=True, exist_ok=True)
            file_obj = tf.extractfile(member)
            if file_obj is None:
                raise ValueError(f"failed to extract tar entry: {member.name}")
            data = file_obj.read()
            out_path.write_bytes(data)
            extracted.append(
                {
                    "path": out_path.relative_to(out_dir).as_posix(),
                    "sha256": hashlib.sha256(data).hexdigest(),
                    "size": len(data),
                }
            )

    extracted.sort(key=lambda e: e["path"])
    return extracted


def _detect_stdlib_version(toolchain_repo: Path) -> str:
    std_dir = toolchain_repo / "stdlib" / "std"
    versions = []
    for child in std_dir.iterdir():
        if not child.is_dir():
            continue
        parsed = _parse_semver(child.name)
        if parsed is None:
            continue
        versions.append((parsed, child.name))
    if not versions:
        raise ValueError(f"no stdlib versions found under {std_dir}")
    versions.sort()
    return versions[-1][1]


def _generate_stdlib_index(toolchain_repo: Path, out_path: Path) -> dict:
    stdlib_version = _detect_stdlib_version(toolchain_repo)
    modules_root = toolchain_repo / "stdlib" / "std" / stdlib_version / "modules"
    module_paths = sorted(modules_root.rglob("*.x07.json"))

    modules: list[dict] = []
    for p in module_paths:
        doc = json.loads(p.read_text(encoding="utf-8"))
        module_id = doc.get("module_id")
        if not isinstance(module_id, str) or not module_id:
            raise ValueError(f"stdlib module missing module_id: {p}")
        exports: list[str] = []
        for decl in doc.get("decls", []):
            if not isinstance(decl, dict):
                continue
            if decl.get("kind") == "export":
                names = decl.get("names", [])
                if isinstance(names, list):
                    exports.extend([n for n in names if isinstance(n, str)])
        exports = sorted(set(exports))
        modules.append(
            {
                "module_id": module_id,
                "path": p.relative_to(modules_root).as_posix(),
                "exports": exports,
            }
        )

    modules.sort(key=lambda m: m["module_id"])
    idx = {
        "schema_version": "x07.website.stdlib-index@0.1.0",
        "stdlib_version": stdlib_version,
        "modules": modules,
    }
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(idx, indent=2) + "\n", encoding="utf-8")
    return idx


def _copy_tree(src: Path, dst: Path) -> None:
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def _sync_agent_portal(
    toolchain_version: str,
    docs_bundle_sha256: str,
    toolchain_repo: Path,
    out_dir: Path,
) -> list[dict]:
    if out_dir.exists():
        shutil.rmtree(out_dir)
    (out_dir / "schemas").mkdir(parents=True, exist_ok=True)
    (out_dir / "skills").mkdir(parents=True, exist_ok=True)
    (out_dir / "stdlib").mkdir(parents=True, exist_ok=True)
    (out_dir / "examples").mkdir(parents=True, exist_ok=True)

    # Schemas
    schema_srcs = [
        toolchain_repo / "spec",
        toolchain_repo / "schemas",
    ]
    copied: list[dict] = []
    for src in schema_srcs:
        if not src.is_dir():
            continue
        for p in sorted(src.glob("*.schema.json")):
            data = p.read_bytes()
            dst = out_dir / "schemas" / p.name
            dst.write_bytes(data)
            copied.append(
                {
                    "path": dst.relative_to(out_dir).as_posix(),
                    "sha256": hashlib.sha256(data).hexdigest(),
                    "size": len(data),
                }
            )

    # Skills (raw folder sync)
    skills_src = toolchain_repo / "skills"
    if skills_src.is_dir():
        shutil.copytree(skills_src, out_dir / "skills", dirs_exist_ok=True)
        for p in sorted((out_dir / "skills").rglob("*")):
            if not p.is_file():
                continue
            data = p.read_bytes()
            copied.append(
                {
                    "path": p.relative_to(out_dir).as_posix(),
                    "sha256": hashlib.sha256(data).hexdigest(),
                    "size": len(data),
                }
            )

    # Stdlib index
    stdlib_index_path = out_dir / "stdlib" / "index.json"
    stdlib_index = _generate_stdlib_index(toolchain_repo, stdlib_index_path)
    copied.append(
        {
            "path": stdlib_index_path.relative_to(out_dir).as_posix(),
            "sha256": hashlib.sha256(
                stdlib_index_path.read_bytes()
            ).hexdigest(),
            "size": stdlib_index_path.stat().st_size,
        }
    )

    # Examples (a small, stable subset)
    examples_src = toolchain_repo / "examples"
    if examples_src.is_dir():
        for p in sorted(examples_src.glob("*.x07.json")):
            data = p.read_bytes()
            dst = out_dir / "examples" / p.name
            dst.write_bytes(data)
            copied.append(
                {
                    "path": dst.relative_to(out_dir).as_posix(),
                    "sha256": hashlib.sha256(data).hexdigest(),
                    "size": len(data),
                }
            )

    copied.sort(key=lambda e: e["path"])
    agent_index = {
        "schema_version": "x07.website.agent-index@0.1.0",
        "toolchain_version": toolchain_version,
        "docs_bundle_sha256": docs_bundle_sha256,
        "schemas_dir": "schemas/",
        "skills_dir": "skills/",
        "stdlib_index_url": "stdlib/index.json",
        "examples_dir": "examples/",
        "manifest_url": "manifest.json",
    }
    index_path = out_dir / "index.json"
    index_path.write_text(json.dumps(agent_index, indent=2) + "\n", encoding="utf-8")
    copied.append(
        {
            "path": index_path.relative_to(out_dir).as_posix(),
            "sha256": hashlib.sha256(index_path.read_bytes()).hexdigest(),
            "size": index_path.stat().st_size,
        }
    )
    copied.sort(key=lambda e: e["path"])
    return copied


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    toolchain_version = args.toolchain_version
    if _parse_semver(toolchain_version) is None:
        print(f"ERROR: invalid toolchain version: {toolchain_version}", file=sys.stderr)
        return 2

    root = Path(__file__).resolve().parents[1]
    bundle_path = args.bundle
    if not bundle_path.is_file():
        print(f"ERROR: bundle not found: {bundle_path}", file=sys.stderr)
        return 2
    docs_bundle_sha256 = _sha256_path(bundle_path)

    toolchain_repo = args.toolchain_repo
    if toolchain_repo is None:
        toolchain_repo = root.parent / "x07"
    toolchain_repo = toolchain_repo.resolve()
    if not toolchain_repo.is_dir():
        print(f"ERROR: toolchain repo not found: {toolchain_repo}", file=sys.stderr)
        return 2

    version_dir = f"v{toolchain_version}"
    docs_out = root / "docs" / version_dir
    agent_out = root / "agent" / version_dir

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        tmp_docs = tmp_path / "docs"
        extracted_docs = _extract_human_docs_bundle(bundle_path, tmp_docs)
        if docs_out.exists():
            shutil.rmtree(docs_out)
        shutil.copytree(tmp_docs, docs_out)

    synced_agent_files = _sync_agent_portal(
        toolchain_version=toolchain_version,
        docs_bundle_sha256=docs_bundle_sha256,
        toolchain_repo=toolchain_repo,
        out_dir=agent_out,
    )

    # Copy latest pointers
    _copy_tree(docs_out, root / "docs" / "latest")
    _copy_tree(agent_out, root / "agent" / "latest")

    # Persist manifests (machine-first)
    manifest = {
        "schema_version": "x07.website.docs-bundle-manifest@0.1.0",
        "toolchain_version": toolchain_version,
        "docs_bundle_sha256": docs_bundle_sha256,
        "human_docs": extracted_docs,
        "agent": synced_agent_files,
    }
    (root / "agent" / version_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    _copy_tree(agent_out, root / "agent" / "latest")

    # Update versions map
    versions_path = root / "versions" / "toolchain_versions.json"
    versions = json.loads(versions_path.read_text(encoding="utf-8"))
    if versions.get("schema_version") != "x07.website.toolchain-versions@0.1.0":
        raise ValueError("versions schema_version mismatch")
    versions_list = versions.get("versions", [])
    if not isinstance(versions_list, list):
        raise ValueError("versions.versions must be array")

    updated = False
    for entry in versions_list:
        if not isinstance(entry, dict):
            continue
        if entry.get("toolchain_version") == toolchain_version:
            entry["docs_bundle_sha256"] = docs_bundle_sha256
            entry["published_at_utc"] = args.published_at_utc
            updated = True
            break
    if not updated:
        versions_list.append(
            {
                "toolchain_version": toolchain_version,
                "docs_bundle_sha256": docs_bundle_sha256,
                "published_at_utc": args.published_at_utc,
            }
        )
    versions["versions"] = _sort_toolchain_versions_desc(versions_list)
    if args.set_latest or versions.get("latest_toolchain_version") is None:
        versions["latest_toolchain_version"] = toolchain_version
    versions_path.write_text(json.dumps(versions, indent=2) + "\n", encoding="utf-8")

    print(f"ok: synced docs+agent for {toolchain_version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
