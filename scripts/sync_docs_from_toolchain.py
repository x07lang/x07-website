from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def parse_args(argv: list[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--toolchain-repo",
        type=Path,
        default=None,
        help="Path to the x07 toolchain repo (default: sibling ../x07)",
    )
    ap.add_argument(
        "--toolchain-version",
        default=None,
        help="Target docs/vX.Y.Z to update (default: versions/toolchain_versions.json latest_toolchain_version)",
    )
    ap.add_argument("--check", action="store_true", help="Fail if outputs are out of date")
    return ap.parse_args(argv)


def infer_toolchain_repo(website_root: Path) -> Path | None:
    candidates = [
        website_root.parent / "x07",
        website_root.parent,
    ]
    for candidate in candidates:
        if not candidate.is_dir():
            continue
        if not (candidate / "Cargo.toml").is_file():
            continue
        if not (candidate / "docs").is_dir():
            continue
        return candidate.resolve()
    return None


def _iter_tracked_docs_files(*, toolchain_repo: Path, docs_root: Path) -> list[tuple[Path, Path]]:
    try:
        rel_docs = docs_root.relative_to(toolchain_repo)
    except ValueError as exc:
        raise ValueError(f"docs root is not inside toolchain repo: {docs_root}") from exc

    proc = subprocess.run(
        ["git", "-C", str(toolchain_repo), "ls-files", "-z", "--", rel_docs.as_posix()],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if proc.returncode != 0:
        raise ValueError(f"git ls-files failed for {toolchain_repo} (is it a git checkout?)")

    raw = proc.stdout
    if not raw:
        raise ValueError(f"no tracked docs files under {docs_root} (git ls-files returned empty)")

    files: list[tuple[Path, Path]] = []
    for item in raw.split(b"\0"):
        if not item:
            continue
        rel_repo = Path(item.decode("utf-8"))
        try:
            rel = rel_repo.relative_to(rel_docs)
        except ValueError:
            continue

        if rel.name == ".DS_Store":
            continue
        if any(part.startswith("._") for part in rel.parts):
            continue

        src = toolchain_repo / rel_repo
        if not src.is_file():
            continue
        files.append((rel, src))

    files.sort(key=lambda e: e[0].as_posix())
    return files


def _materialize_docs_tree(*, toolchain_repo: Path, docs_root: Path, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    for rel, src in _iter_tracked_docs_files(toolchain_repo=toolchain_repo, docs_root=docs_root):
        dst = out_dir / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(src.read_bytes())


def _apply_docs_overlays(*, repo_root: Path, docs_out: Path) -> None:
    overlays_root = repo_root / "docs" / "_overlays"
    if not overlays_root.is_dir():
        return

    for src in sorted(overlays_root.rglob("*")):
        if not src.is_file():
            continue
        rel = src.relative_to(overlays_root)
        if rel.name == ".DS_Store":
            continue
        if any(part.startswith("._") for part in rel.parts):
            continue

        dst = docs_out / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(src.read_bytes())


def _iter_files_sorted(root: Path) -> list[Path]:
    out: list[Path] = []
    for p in root.rglob("*"):
        if not p.is_file():
            continue
        rel = p.relative_to(root)
        if rel == Path("_bundle_meta.json"):
            continue
        if rel.name == ".DS_Store":
            continue
        if any(part.startswith("._") for part in rel.parts):
            continue
        out.append(rel)
    return sorted(out, key=lambda p: p.as_posix())


def _compare_trees(*, a: Path, b: Path, label: str) -> None:
    a_files = _iter_files_sorted(a)
    b_files = _iter_files_sorted(b)
    if a_files != b_files:
        raise ValueError(f"tree mismatch ({label}): {a} vs {b}")
    for rel in a_files:
        if (a / rel).read_bytes() != (b / rel).read_bytes():
            raise ValueError(f"file differs ({label}): {rel.as_posix()}")


def _load_latest_toolchain_version(versions_path: Path) -> str | None:
    doc = json.loads(versions_path.read_text(encoding="utf-8"))
    if not isinstance(doc, dict):
        raise ValueError(f"expected JSON object: {versions_path}")
    latest = doc.get("latest_toolchain_version")
    if latest is None:
        return None
    if not isinstance(latest, str) or not latest:
        raise ValueError(f"latest_toolchain_version must be string|null: {versions_path}")
    return latest


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    website_root = Path.cwd()

    versions_path = website_root / "versions" / "toolchain_versions.json"
    if not versions_path.is_file():
        print(f"ERROR: missing {versions_path}", file=sys.stderr)
        return 2

    toolchain_version = args.toolchain_version
    if toolchain_version is None:
        toolchain_version = _load_latest_toolchain_version(versions_path)
    if toolchain_version is None:
        print(
            "ERROR: toolchain version unspecified and versions/toolchain_versions.json has latest_toolchain_version=null",
            file=sys.stderr,
        )
        return 2

    toolchain_repo = args.toolchain_repo
    if toolchain_repo is None:
        toolchain_repo = infer_toolchain_repo(website_root)
    if toolchain_repo is None:
        print("ERROR: unable to locate toolchain repo (pass --toolchain-repo)", file=sys.stderr)
        return 2
    toolchain_repo = toolchain_repo.resolve()

    src_docs = toolchain_repo / "docs"
    if not (src_docs / "SUMMARY.md").is_file():
        print(f"ERROR: expected toolchain docs root at {src_docs} (missing SUMMARY.md)", file=sys.stderr)
        return 2

    dst_version_dir = website_root / "docs" / f"v{toolchain_version}"
    dst_latest_dir = website_root / "docs" / "latest"
    preserved_meta: bytes | None = None
    preserved_meta_path = dst_version_dir / "_bundle_meta.json"
    if preserved_meta_path.is_file():
        preserved_meta = preserved_meta_path.read_bytes()

    if args.check:
        with tempfile.TemporaryDirectory() as tmp:
            expected_dir = Path(tmp) / "docs"
            _materialize_docs_tree(toolchain_repo=toolchain_repo, docs_root=src_docs, out_dir=expected_dir)
            _apply_docs_overlays(repo_root=website_root, docs_out=expected_dir)
            _compare_trees(a=expected_dir, b=dst_version_dir, label=f"docs/v{toolchain_version}")
        _compare_trees(a=dst_version_dir, b=dst_latest_dir, label="docs/latest")
    else:
        if dst_version_dir.exists():
            shutil.rmtree(dst_version_dir)
        _materialize_docs_tree(toolchain_repo=toolchain_repo, docs_root=src_docs, out_dir=dst_version_dir)
        _apply_docs_overlays(repo_root=website_root, docs_out=dst_version_dir)
        if preserved_meta is not None and not preserved_meta_path.exists():
            preserved_meta_path.write_bytes(preserved_meta)
        if dst_latest_dir.exists():
            shutil.rmtree(dst_latest_dir)
        shutil.copytree(dst_version_dir, dst_latest_dir)

    gen_inputs = website_root / "scripts" / "site" / "gen_docusaurus_inputs.py"
    subprocess.check_call(
        [
            sys.executable,
            str(gen_inputs),
            "--repo-root",
            str(website_root),
            *(["--check"] if args.check else []),
        ]
    )
    if args.check:
        subprocess.check_call([sys.executable, str(website_root / "scripts" / "check_site.py"), "--check"])

    print(f"ok: synced docs/latest (and docs/v{toolchain_version}) from {src_docs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
