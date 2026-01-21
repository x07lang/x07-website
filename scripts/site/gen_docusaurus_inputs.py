#!/usr/bin/env python3
"""
Generate Docusaurus inputs from the repo's synced docs/ tree.

Inputs (synced by bundle):
- ./docs/latest/**            (latest human docs, includes SUMMARY.md)
- ./docs/vX.Y.Z/**            (historical human docs, includes SUMMARY.md)
- ./versions/toolchain_versions.json

Outputs (generated inside ./site/, NOT edited manually):
- ./site/docs/**                              (current docs = docs/latest)
- ./site/versions.json                        (list of version names newest->oldest)
- ./site/versioned_docs/version-X.Y.Z/**      (docs/vX.Y.Z content)
- ./site/versioned_sidebars/version-X.Y.Z-sidebars.json
- ./site/sidebars.generated.ts                (generated; deterministic order from SUMMARY.md)
- ./site/redirects.generated.ts               (generated; legacy /docs/latest + /docs/vX.Y.Z redirects)
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
from typing import Any, Callable, Dict, Iterable, List, Tuple

from _summary_to_sidebar import (
    SummaryParseError,
    parse_summary_md,
    summary_nodes_to_sidebar_items,
)


_SEMVER_RE = re.compile(r"^(?P<maj>0|[1-9]\d*)\.(?P<min>0|[1-9]\d*)\.(?P<pat>0|[1-9]\d*)$")


def _semver_key(v: str) -> Tuple[int, int, int]:
    m = _SEMVER_RE.match(v)
    if not m:
        raise ValueError(f"unsupported version format (expected X.Y.Z): {v}")
    return (int(m.group("maj")), int(m.group("min")), int(m.group("pat")))


def _read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _write_text_if_changed(path: Path, content: str, check: bool) -> None:
    content = content.replace("\r\n", "\n")
    if not content.endswith("\n"):
        content += "\n"

    if path.exists():
        existing = path.read_text(encoding="utf-8").replace("\r\n", "\n")
        if existing == content:
            return
        if check:
            raise SystemExit(f"[CHECK] {path} is out of date (run gen_docusaurus_inputs.py).")
    else:
        if check:
            raise SystemExit(f"[CHECK] {path} missing (run gen_docusaurus_inputs.py).")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _write_json_if_changed(path: Path, obj: Any, check: bool) -> None:
    content = json.dumps(obj, ensure_ascii=False, indent=2, sort_keys=True)
    _write_text_if_changed(path, content, check)


def _safe_rmtree(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)


def _iter_files_sorted(root: Path) -> List[Path]:
    out: List[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        filenames.sort()
        dir_p = Path(dirpath)
        for fn in filenames:
            out.append((dir_p / fn).relative_to(root))
    out.sort()
    return out


def _sync_tree(
    *,
    src: Path,
    dst: Path,
    check: bool,
    exclude_relpath: Callable[[Path], bool],
) -> None:
    if not src.is_dir():
        raise SystemExit(f"source directory missing: {src}")

    expected: List[Path] = [p for p in _iter_files_sorted(src) if not exclude_relpath(p)]

    if check:
        if not dst.is_dir():
            raise SystemExit(f"[CHECK] expected directory missing: {dst}")
        actual: List[Path] = [p for p in _iter_files_sorted(dst) if not exclude_relpath(p)]
        if actual != expected:
            raise SystemExit(f"[CHECK] directory out of date: {dst}")
        for rel in expected:
            a = (src / rel).read_bytes()
            b = (dst / rel).read_bytes()
            if a != b:
                raise SystemExit(f"[CHECK] file differs: {dst / rel}")
        return

    _safe_rmtree(dst)
    dst.mkdir(parents=True, exist_ok=True)
    for rel in expected:
        src_file = src / rel
        dst_file = dst / rel
        dst_file.parent.mkdir(parents=True, exist_ok=True)
        dst_file.write_bytes(src_file.read_bytes())


def _read_toolchain_versions(versions_path: Path) -> List[str]:
    data = _read_json(versions_path)
    if not isinstance(data, dict):
        raise SystemExit(f"invalid JSON object: {versions_path}")
    entries = data.get("versions")
    if not isinstance(entries, list):
        raise SystemExit(f"invalid versions/toolchain_versions.json: versions must be a list")
    out: List[str] = []
    for entry in entries:
        if not isinstance(entry, dict):
            raise SystemExit(f"invalid versions entry (expected object): {entry}")
        toolchain_version = entry.get("toolchain_version")
        if not isinstance(toolchain_version, str) or not toolchain_version:
            raise SystemExit("invalid versions entry: toolchain_version must be non-empty string")
        out.append(toolchain_version)

    # newest -> oldest
    out = sorted(out, key=_semver_key, reverse=True)
    return out


def _exclude_docs_relpath(rel: Path) -> bool:
    if rel.name == "SUMMARY.md":
        return True
    if "_generated" in rel.parts:
        return True
    return False


def _generate_sidebar_items(summary_path: Path) -> List[Dict[str, Any]]:
    text = _read_text(summary_path)
    nodes = parse_summary_md(text)
    return summary_nodes_to_sidebar_items(nodes)


def _generate_sidebars_ts(items: List[Dict[str, Any]]) -> str:
    rendered_items = json.dumps(items, ensure_ascii=False, indent=2)
    return f"""\
/**
 * THIS FILE IS GENERATED by scripts/site/gen_docusaurus_inputs.py
 * DO NOT EDIT BY HAND.
 */
import type {{SidebarsConfig}} from '@docusaurus/plugin-content-docs';

const sidebars: SidebarsConfig = {{
  docs: {rendered_items},
}};

export default sidebars;
"""


def _generate_redirects_ts(toolchain_versions: List[str]) -> str:
    rendered_versions = json.dumps(toolchain_versions, ensure_ascii=False, indent=2)
    return f"""\
/**
 * THIS FILE IS GENERATED by scripts/site/gen_docusaurus_inputs.py
 * DO NOT EDIT BY HAND.
 */

export const TOOLCHAIN_VERSIONS: readonly string[] = {rendered_versions} as const;

function isVersionedDocsPath(existingPath: string): boolean {{
  for (const v of TOOLCHAIN_VERSIONS) {{
    if (existingPath === `/docs/${{v}}` || existingPath.startsWith(`/docs/${{v}}/`)) {{
      return true;
    }}
  }}
  return false;
}}

export function createRedirects(existingPath: string): string[] | undefined {{
  // Docusaurus may expose both "/x" and "/x/" as existing paths. If we create
  // redirects for both, they collide on the same output file (x/index.html).
  //
  // Base docs index pages often come through as "/docs/" and "/docs/<ver>/".
  // We special-case those, then ignore the remaining trailing-slash variants.
  if (existingPath === '/docs/') {{
    return ['/docs/latest'];
  }}
  for (const v of TOOLCHAIN_VERSIONS) {{
    const base = `/docs/${{v}}`;
    if (existingPath === `${{base}}/`) {{
      return [`/docs/v${{v}}`];
    }}
  }}
  if (existingPath !== '/' && existingPath.endsWith('/')) {{
    return undefined;
  }}

  // Current docs legacy URLs: /docs/latest/*
  if (existingPath === '/docs') {{
    return undefined;
  }}
  if (existingPath.startsWith('/docs/') && !isVersionedDocsPath(existingPath)) {{
    const redirects = [`/docs/latest${{existingPath.slice('/docs'.length)}}`];

    if (existingPath === '/docs/getting-started/install') {{
      redirects.push('/install');
    }}
    if (existingPath === '/docs/toolchain/cli') {{
      redirects.push('/docs/cli');
    }}
    if (existingPath === '/docs/worlds/os-worlds') {{
      redirects.push('/docs/worlds/os');
    }}

    return redirects;
  }}

  // Versioned docs legacy URLs: /docs/vX.Y.Z/*
  for (const v of TOOLCHAIN_VERSIONS) {{
    const base = `/docs/${{v}}`;
    if (existingPath === base) {{
      return undefined;
    }}
    if (existingPath.startsWith(`${{base}}/`)) {{
      return [`/docs/v${{v}}/${{existingPath.slice(`${{base}}/`.length)}}`];
    }}
  }}

  return undefined;
}}
"""


def parse_args(argv: List[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", default=".", help="x07-website repo root")
    ap.add_argument("--check", action="store_true", help="fail if outputs are not up to date")
    return ap.parse_args(argv)


def main(argv: List[str]) -> None:
    args = parse_args(argv)
    repo = Path(args.repo_root).resolve()

    docs_root = repo / "docs"
    versions_path = repo / "versions" / "toolchain_versions.json"
    site_root = repo / "site"

    src_latest = docs_root / "latest"
    if not src_latest.is_dir():
        raise SystemExit(f"missing docs/latest at {src_latest}")

    if not versions_path.is_file():
        raise SystemExit(f"missing versions/toolchain_versions.json at {versions_path}")

    toolchain_versions = _read_toolchain_versions(versions_path)

    site_docs = site_root / "docs"
    site_versioned_docs = site_root / "versioned_docs"
    site_versioned_sidebars = site_root / "versioned_sidebars"
    site_versions_json = site_root / "versions.json"
    site_sidebars_ts = site_root / "sidebars.generated.ts"
    site_redirects_ts = site_root / "redirects.generated.ts"

    # ---- docs ----
    _sync_tree(
        src=src_latest,
        dst=site_docs,
        check=args.check,
        exclude_relpath=_exclude_docs_relpath,
    )

    # ---- versioned docs ----
    for v in toolchain_versions:
        src_ver_dir = docs_root / f"v{v}"
        dst_ver_dir = site_versioned_docs / f"version-{v}"
        _sync_tree(
            src=src_ver_dir,
            dst=dst_ver_dir,
            check=args.check,
            exclude_relpath=_exclude_docs_relpath,
        )

    if not args.check:
        # Remove stale versions that are no longer present in toolchain_versions.json
        expected_dirs = {f"version-{v}" for v in toolchain_versions}
        if site_versioned_docs.is_dir():
            for child in site_versioned_docs.iterdir():
                if child.is_dir() and child.name not in expected_dirs:
                    shutil.rmtree(child)
        if site_versioned_sidebars.is_dir():
            for child in site_versioned_sidebars.iterdir():
                if child.is_file() and child.name.startswith("version-") and child.name.endswith("-sidebars.json"):
                    ver = child.name.removeprefix("version-").removesuffix("-sidebars.json")
                    if ver not in toolchain_versions:
                        child.unlink()

    _write_json_if_changed(site_versions_json, toolchain_versions, check=args.check)

    # ---- sidebars ----
    latest_summary = src_latest / "SUMMARY.md"
    if not latest_summary.is_file():
        raise SystemExit(f"missing SUMMARY.md in docs/latest: {latest_summary}")
    try:
        latest_items = _generate_sidebar_items(latest_summary)
    except SummaryParseError as e:
        raise SystemExit(f"SUMMARY parse error in {latest_summary}: {e}")
    _write_text_if_changed(site_sidebars_ts, _generate_sidebars_ts(latest_items), check=args.check)

    for v in toolchain_versions:
        src_ver_summary = docs_root / f"v{v}" / "SUMMARY.md"
        if not src_ver_summary.is_file():
            raise SystemExit(f"missing SUMMARY.md in docs/v{v}: {src_ver_summary}")
        try:
            ver_items = _generate_sidebar_items(src_ver_summary)
        except SummaryParseError as e:
            raise SystemExit(f"SUMMARY parse error in {src_ver_summary}: {e}")
        out_json = site_versioned_sidebars / f"version-{v}-sidebars.json"
        _write_json_if_changed(out_json, {"docs": ver_items}, check=args.check)

    _write_text_if_changed(site_redirects_ts, _generate_redirects_ts(toolchain_versions), check=args.check)

    if not args.check:
        print("[ok] Generated Docusaurus inputs:")
        for p in [
            site_docs,
            site_versions_json,
            site_versioned_docs,
            site_versioned_sidebars,
            site_sidebars_ts,
            site_redirects_ts,
        ]:
            print(f"  - {p.relative_to(repo)}")


if __name__ == "__main__":
    main(os.sys.argv[1:])
