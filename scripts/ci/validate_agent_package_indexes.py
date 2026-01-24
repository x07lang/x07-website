#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, Iterable


_EXPECTED_INSTANCE_SCHEMA_VERSION = "x07.website.package-index@0.2.0"

# Keep these patterns in sync with agent/latest/schemas/x07-website.package-index.schema.json
_RE_SEMVER = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+([+-][0-9A-Za-z.-]+)?$")
_RE_PKG_NAME = re.compile(r"^[a-z][a-z0-9_-]{0,127}$")
_RE_PKG_REF = re.compile(r"^[a-z][a-z0-9_-]{0,127}@[0-9]+\.[0-9]+\.[0-9]+([+-][0-9A-Za-z.-]+)?$")
_RE_CAPABILITY = re.compile(r"^[a-z][a-z0-9._-]{0,63}$")
_RE_FFI_LIB = re.compile(r"^[A-Za-z0-9_.+-]+$")
_RE_MODULE_ID = re.compile(r"^[a-z][a-z0-9_]*(\.[a-z0-9_]+)*$")
_RE_EXPORT_NAME = re.compile(r"^[A-Za-z0-9_]+(\.[A-Za-z0-9_]+)*$")
_RE_REL_PATH = re.compile(r"^(?![\\/])(?![A-Za-z]:)(?!.*(?:^|[\\/])\.\.(?:[\\/]|$)).+$")
_RE_REL_DIR_EXTRA = re.compile(r"^(?!.*\.x07\.json$).+$")

_WORLD_IDS = {
    "solve-pure",
    "solve-fs",
    "solve-rr",
    "solve-kv",
    "solve-full",
    "run-os",
    "run-os-sandboxed",
}
_DETERMINISM_TIERS = {"pure", "os-world-only"}
_IMPORT_MODES = {"handwritten", "x07import", "ffi"}
_VISIBILITY = {"canonical", "advanced", "experimental"}


def parse_args(argv: list[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true", help="run validation (required)")
    ap.add_argument(
        "--repo-root",
        type=Path,
        default=None,
        help="repo root (defaults to scripts/ci/../../)",
    )
    ap.add_argument(
        "--agent-dir",
        action="append",
        default=[],
        help="agent portal dir to validate (repeatable). Default: agent/latest",
    )
    ap.add_argument(
        "--expected-schema-version",
        default=_EXPECTED_INSTANCE_SCHEMA_VERSION,
        help="expected schema_version field value in instances",
    )
    ap.add_argument(
        "--glob",
        default="packages/*/*/index.json",
        help="glob pattern under each --agent-dir to find package index files",
    )
    ap.add_argument(
        "--max-errors",
        type=int,
        default=200,
        help="cap number of printed errors (prevents log spam)",
    )
    ap.add_argument(
        "--json-report",
        type=Path,
        default=None,
        help="optional: write a machine-readable report JSON to this path",
    )
    return ap.parse_args(argv)


def _read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as e:
        raise ValueError(f"{path}: invalid JSON: {e}")


def _json_pointer(path: Iterable[Any]) -> str:
    parts: list[str] = []
    for p in path:
        if isinstance(p, int):
            parts.append(str(p))
        else:
            s = str(p)
            s = s.replace("~", "~0").replace("/", "~1")
            parts.append(s)
    return "/" + "/".join(parts) if parts else "/"


def _rel(root: Path, p: Path) -> str:
    try:
        return p.relative_to(root).as_posix()
    except Exception:
        return str(p)


def _is_str(x: Any) -> bool:
    return isinstance(x, str)


def _is_nonempty_str(x: Any) -> bool:
    return isinstance(x, str) and bool(x)


def _validate_rel_dir(path: Any) -> str | None:
    if not _is_nonempty_str(path):
        return "must be non-empty string"
    if len(path) > 1024:
        return "too long"
    if _RE_REL_PATH.match(path) is None:
        return "must be relative path (no absolute paths, no '..')"
    if _RE_REL_DIR_EXTRA.match(path) is None:
        return "must be a directory path (not a .x07.json file)"
    return None


def _validate_rel_module_path(path: Any) -> str | None:
    if not _is_nonempty_str(path):
        return "must be non-empty string"
    if len(path) > 1024:
        return "too long"
    if _RE_REL_PATH.match(path) is None:
        return "must be relative path (no absolute paths, no '..')"
    if not path.endswith(".x07.json"):
        return "must end with .x07.json"
    return None


def _validate_pkg_name(name: Any) -> str | None:
    if not _is_nonempty_str(name):
        return "must be non-empty string"
    if len(name) > 128:
        return "too long"
    if _RE_PKG_NAME.match(name) is None:
        return "invalid package name"
    return None


def _validate_semver(version: Any) -> str | None:
    if not _is_nonempty_str(version):
        return "must be non-empty string"
    if len(version) > 64:
        return "too long"
    if _RE_SEMVER.match(version) is None:
        return "invalid semver"
    return None


def _validate_pkg_ref(ref: Any) -> str | None:
    if not _is_nonempty_str(ref):
        return "must be non-empty string"
    if len(ref) > 256:
        return "too long"
    if _RE_PKG_REF.match(ref) is None:
        return "invalid pkg ref (expected name@version)"
    return None


def _validate_capability(cap: Any) -> str | None:
    if not _is_nonempty_str(cap):
        return "must be non-empty string"
    if len(cap) > 64:
        return "too long"
    if _RE_CAPABILITY.match(cap) is None:
        return "invalid capability id"
    return None


def _validate_ffi_lib(lib: Any) -> str | None:
    if not _is_nonempty_str(lib):
        return "must be non-empty string"
    if len(lib) > 128:
        return "too long"
    if _RE_FFI_LIB.match(lib) is None:
        return "invalid ffi lib token"
    return None


def _validate_module_id(mid: Any) -> str | None:
    if not _is_nonempty_str(mid):
        return "must be non-empty string"
    if len(mid) > 256:
        return "too long"
    if _RE_MODULE_ID.match(mid) is None:
        return "invalid module_id"
    return None


def _validate_export_name(n: Any) -> str | None:
    if not _is_nonempty_str(n):
        return "must be non-empty string"
    if len(n) > 256:
        return "too long"
    if _RE_EXPORT_NAME.match(n) is None:
        return "invalid export name"
    return None


def _validate_string_max(x: Any, max_len: int) -> str | None:
    if not isinstance(x, str):
        return "must be a string"
    if len(x) > max_len:
        return f"too long (max {max_len})"
    return None


def _validate_unique_str_array(
    items: Any, *, item_validator, allow_empty: bool = False
) -> tuple[str | None, list[str] | None]:
    if not isinstance(items, list):
        return "must be an array", None
    if not allow_empty and not items:
        return "must be non-empty", None
    out: list[str] = []
    seen: set[str] = set()
    for it in items:
        msg = item_validator(it)
        if msg is not None:
            return msg, None
        assert isinstance(it, str)
        if it in seen:
            return "must have unique items", None
        seen.add(it)
        out.append(it)
    return None, out


def _validate_meta(meta: Any) -> list[dict]:
    errors: list[dict] = []
    if not isinstance(meta, dict):
        return [{"pointer": "/meta", "message": "must be an object"}]

    allowed = {
        "determinism_tier",
        "worlds_allowed",
        "import_mode",
        "requires_packages",
        "ffi_libs",
        "capabilities",
        "visibility",
    }
    extra = sorted(k for k in meta.keys() if k not in allowed)
    if extra:
        errors.append({"pointer": "/meta", "message": f"unexpected keys: {extra}"})

    tier = meta.get("determinism_tier")
    if tier not in _DETERMINISM_TIERS:
        errors.append({"pointer": "/meta/determinism_tier", "message": f"must be one of {sorted(_DETERMINISM_TIERS)}"})

    worlds = meta.get("worlds_allowed")
    if not isinstance(worlds, list) or not worlds:
        errors.append({"pointer": "/meta/worlds_allowed", "message": "must be a non-empty array"})
    else:
        seen: set[str] = set()
        for i, w in enumerate(worlds):
            if not _is_nonempty_str(w) or w not in _WORLD_IDS:
                errors.append({"pointer": f"/meta/worlds_allowed/{i}", "message": f"invalid world id: {w!r}"})
                continue
            if w in seen:
                errors.append({"pointer": "/meta/worlds_allowed", "message": "must have unique items"})
                break
            seen.add(w)

    import_mode = meta.get("import_mode")
    if import_mode not in _IMPORT_MODES:
        errors.append({"pointer": "/meta/import_mode", "message": f"must be one of {sorted(_IMPORT_MODES)}"})

    visibility = meta.get("visibility")
    if visibility is not None and visibility not in _VISIBILITY:
        errors.append({"pointer": "/meta/visibility", "message": f"must be one of {sorted(_VISIBILITY)}"})

    reqs = meta.get("requires_packages")
    if reqs is not None:
        msg, _ = _validate_unique_str_array(reqs, item_validator=_validate_pkg_ref, allow_empty=True)
        if msg is not None:
            errors.append({"pointer": "/meta/requires_packages", "message": msg})

    ffi_libs = meta.get("ffi_libs")
    if ffi_libs is not None:
        msg, _ = _validate_unique_str_array(ffi_libs, item_validator=_validate_ffi_lib, allow_empty=True)
        if msg is not None:
            errors.append({"pointer": "/meta/ffi_libs", "message": msg})

    caps = meta.get("capabilities")
    if caps is not None:
        msg, _ = _validate_unique_str_array(caps, item_validator=_validate_capability, allow_empty=True)
        if msg is not None:
            errors.append({"pointer": "/meta/capabilities", "message": msg})

    return errors


def _validate_modules(mods: Any) -> list[dict]:
    errors: list[dict] = []
    if not isinstance(mods, list) or not mods:
        return [{"pointer": "/modules", "message": "must be a non-empty array"}]

    for i, m in enumerate(mods):
        pfx = f"/modules/{i}"
        if not isinstance(m, dict):
            errors.append({"pointer": pfx, "message": "must be an object"})
            continue

        allowed = {"module_id", "path", "exports"}
        extra = sorted(k for k in m.keys() if k not in allowed)
        if extra:
            errors.append({"pointer": pfx, "message": f"unexpected keys: {extra}"})

        msg = _validate_module_id(m.get("module_id"))
        if msg is not None:
            errors.append({"pointer": f"{pfx}/module_id", "message": msg})

        msg = _validate_rel_module_path(m.get("path"))
        if msg is not None:
            errors.append({"pointer": f"{pfx}/path", "message": msg})

        exports = m.get("exports")
        if not isinstance(exports, list):
            errors.append({"pointer": f"{pfx}/exports", "message": "must be an array"})
        else:
            seen: set[str] = set()
            for j, e in enumerate(exports):
                msg = _validate_export_name(e)
                if msg is not None:
                    errors.append({"pointer": f"{pfx}/exports/{j}", "message": msg})
                    continue
                assert isinstance(e, str)
                if e in seen:
                    errors.append({"pointer": f"{pfx}/exports", "message": "must have unique items"})
                    break
                seen.add(e)

    return errors


def _validate_instance(
    *,
    idx_path: Path,
    expected_schema_version: str,
) -> list[dict]:
    errors: list[dict] = []

    try:
        inst = _read_json(idx_path)
    except ValueError as e:
        return [{"pointer": "/", "message": str(e)}]

    if not isinstance(inst, dict):
        return [{"pointer": "/", "message": "must be a JSON object"}]

    allowed = {"schema_version", "name", "version", "description", "docs", "module_root", "meta", "modules"}
    extra = sorted(k for k in inst.keys() if k not in allowed)
    if extra:
        errors.append({"pointer": "/", "message": f"unexpected keys: {extra}"})

    sv = inst.get("schema_version")
    if sv != expected_schema_version:
        errors.append(
            {
                "pointer": "/schema_version",
                "message": f"schema_version mismatch: expected {expected_schema_version!r}, got {sv!r}",
            }
        )

    msg = _validate_pkg_name(inst.get("name"))
    if msg is not None:
        errors.append({"pointer": "/name", "message": msg})

    msg = _validate_semver(inst.get("version"))
    if msg is not None:
        errors.append({"pointer": "/version", "message": msg})

    msg = _validate_string_max(inst.get("description"), 4096)
    if msg is not None:
        errors.append({"pointer": "/description", "message": msg})

    msg = _validate_string_max(inst.get("docs"), 131072)
    if msg is not None:
        errors.append({"pointer": "/docs", "message": msg})

    msg = _validate_rel_dir(inst.get("module_root"))
    if msg is not None:
        errors.append({"pointer": "/module_root", "message": msg})

    errors.extend(_validate_meta(inst.get("meta")))
    errors.extend(_validate_modules(inst.get("modules")))

    # Path consistency: packages/<name>/<version>/index.json must match instance fields.
    try:
        ver_dir = idx_path.parent.name
        pkg_dir = idx_path.parent.parent.name
    except Exception:
        ver_dir = ""
        pkg_dir = ""

    name = inst.get("name")
    version = inst.get("version")
    if isinstance(name, str) and pkg_dir and name != pkg_dir:
        errors.append({"pointer": "/name", "message": f"name mismatch: expected {pkg_dir!r} from path, got {name!r}"})
    if isinstance(version, str) and ver_dir and version != ver_dir:
        errors.append(
            {
                "pointer": "/version",
                "message": f"version mismatch: expected {ver_dir!r} from path, got {version!r}",
            }
        )

    return errors


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if not args.check:
        print("usage: validate_agent_package_indexes.py --check", file=sys.stderr)
        return 2

    repo_root = args.repo_root
    if repo_root is None:
        repo_root = Path(__file__).resolve().parents[2]
    repo_root = repo_root.resolve()

    agent_dirs = args.agent_dir or ["agent/latest"]

    errors_out: list[dict] = []
    checked_files = 0
    scanned_files = 0

    for agent_dir in agent_dirs:
        agent_root = Path(agent_dir)
        agent_root = agent_root if agent_root.is_absolute() else (repo_root / agent_root)
        agent_root = agent_root.resolve()

        if not agent_root.is_dir():
            errors_out.append({"file": _rel(repo_root, agent_root), "pointer": "/", "message": "agent dir not found"})
            continue

        idx_files = sorted(agent_root.glob(args.glob), key=lambda p: p.as_posix())
        scanned_files += len(idx_files)

        if not idx_files:
            errors_out.append(
                {
                    "file": _rel(repo_root, agent_root),
                    "pointer": "/",
                    "message": f"no files matched glob {args.glob!r}",
                }
            )
            continue

        for idx_path in idx_files:
            if len(errors_out) >= args.max_errors:
                break

            checked_files += 1
            rel_idx = _rel(repo_root, idx_path)
            inst_errors = _validate_instance(idx_path=idx_path, expected_schema_version=args.expected_schema_version)
            for e in inst_errors:
                if len(errors_out) >= args.max_errors:
                    break
                errors_out.append({"file": rel_idx, "pointer": e["pointer"], "message": e["message"]})

    if args.json_report is not None:
        report_path = args.json_report if args.json_report.is_absolute() else (repo_root / args.json_report)
        report = {
            "schema_version": "x07.website.ci.package-index-validation-report@0.1.0",
            "expected_instance_schema_version": args.expected_schema_version,
            "agent_dirs": agent_dirs,
            "glob": args.glob,
            "scanned_files": scanned_files,
            "checked_files": checked_files,
            "errors": errors_out,
            "ok": len(errors_out) == 0,
        }
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    if errors_out:
        for e in errors_out[: args.max_errors]:
            print(f"ERROR: {e['file']}: {e['pointer']}: {e['message']}", file=sys.stderr)
        if len(errors_out) > args.max_errors:
            print(f"ERROR: ... truncated (max-errors={args.max_errors})", file=sys.stderr)
        return 1

    print(f"ok: package index validation ({checked_files} files) against {args.expected_schema_version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
