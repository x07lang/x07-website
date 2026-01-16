#!/usr/bin/env python3
"""
SUMMARY.md -> Docusaurus sidebars model (deterministic).

Supported SUMMARY.md subset:
- Headings: # / ## / ### ... become top-level categories (by heading text)
- Bullet list items:
  - [Label](path/to/doc.md)  => doc item
  - Label (no link)          => category label that may contain nested bullets
Indentation:
- 2+ spaces (or tabs) increases nesting level.

This is intentionally strict and deterministic:
- Unknown constructs raise deterministic exceptions (so CI fails early)
- Output ordering follows SUMMARY.md order (no filesystem scans)
"""

from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Any, Dict, List, Optional, Tuple


SidebarItem = Dict[str, Any]  # {"type": "doc", ...} | {"type":"category", ...}


@dataclass(frozen=True)
class SummaryNode:
    kind: str  # "doc" | "category"
    label: str
    doc_id: Optional[str] = None
    children: Optional[List["SummaryNode"]] = None


_HEADING_RE = re.compile(r"^(?P<h>#{1,6})\s+(?P<title>.+?)\s*$")
_BULLET_RE = re.compile(r"^(?P<indent>[ \t]*)([-*+])\s+(?P<rest>.+?)\s*$")
_LINK_RE = re.compile(r"^\[(?P<label>[^\]]+)\]\((?P<href>[^)]+)\)\s*$")


class SummaryParseError(Exception):
    pass


def _normalize_doc_id(href: str) -> str:
    """
    Convert a Markdown link target into a Docusaurus doc id.

    Rules:
    - Strip query/fragment (if present)
    - Strip leading './'
    - Require relative paths (no http(s)://)
    - Strip .md / .mdx
    - Convert backslashes to forward slashes
    """
    href = href.split("#", 1)[0].split("?", 1)[0].strip()

    if href.startswith("http://") or href.startswith("https://"):
        raise SummaryParseError(f"external link not allowed in SUMMARY.md sidebar: {href}")

    href = href.lstrip("./")
    href = href.replace("\\", "/")

    if href.endswith(".md"):
        href = href[:-3]
    elif href.endswith(".mdx"):
        href = href[:-4]
    else:
        raise SummaryParseError(f"doc link must end with .md or .mdx: {href}")

    if href == "":
        raise SummaryParseError("empty doc id after normalization")

    return href


def parse_summary_md(summary_text: str) -> List[SummaryNode]:
    """
    Parse SUMMARY.md into a list of top-level nodes.

    Strategy:
    - Headings start a new top-level "heading category"
    - Bullets build a tree based on indentation
    - Items before any heading are allowed (they become top-level nodes)
    """
    lines = summary_text.splitlines()

    root: List[SummaryNode] = []
    current_section_title: Optional[str] = None
    current_section_children: List[SummaryNode] = []

    def flush_section() -> None:
        nonlocal current_section_title, current_section_children, root
        if current_section_title is not None:
            root.append(
                SummaryNode(
                    kind="category",
                    label=current_section_title,
                    children=current_section_children,
                )
            )
        else:
            root.extend(current_section_children)
        current_section_title = None
        current_section_children = []

    # Tree builder stacks for bullets inside the *current section*
    stack: List[Tuple[int, List[SummaryNode]]] = [(0, current_section_children)]

    def reset_stack_for_section() -> None:
        nonlocal stack
        stack = [(0, current_section_children)]

    for raw in lines:
        line = raw.rstrip()
        if not line.strip():
            continue

        m_h = _HEADING_RE.match(line)
        if m_h:
            flush_section()
            current_section_title = m_h.group("title").strip()
            current_section_children = []
            reset_stack_for_section()
            continue

        m_b = _BULLET_RE.match(line)
        if not m_b:
            raise SummaryParseError(
                f"unsupported SUMMARY.md line (expected heading or bullet): {line}"
            )

        indent_str = m_b.group("indent")
        indent = 0
        for ch in indent_str:
            indent += 4 if ch == "\t" else 1
        rest = m_b.group("rest").strip()

        level = indent // 2

        while stack and stack[-1][0] > level:
            stack.pop()
        if not stack:
            raise SummaryParseError(f"indentation underflow in SUMMARY.md: {line}")

        if stack[-1][0] < level:
            raise SummaryParseError(
                f"invalid indentation jump (indent under non-category or missing parent bullet): {line}"
            )

        parent_children = stack[-1][1]

        m_l = _LINK_RE.match(rest)
        if m_l:
            label = m_l.group("label").strip()
            href = m_l.group("href").strip()
            doc_id = _normalize_doc_id(href)
            parent_children.append(SummaryNode(kind="doc", label=label, doc_id=doc_id))
        else:
            label = rest
            node = SummaryNode(kind="category", label=label, children=[])
            parent_children.append(node)
            stack.append((level + 1, node.children or []))

    flush_section()
    return root


def _node_to_sidebar_item(node: SummaryNode) -> SidebarItem:
    if node.kind == "doc":
        if not node.doc_id:
            raise ValueError("doc node missing doc_id")
        return {"type": "doc", "id": node.doc_id, "label": node.label}

    if node.kind == "category":
        children = node.children or []
        return {
            "type": "category",
            "label": node.label,
            "collapsed": False,
            "items": [_node_to_sidebar_item(ch) for ch in children],
        }

    raise ValueError(f"unknown SummaryNode kind: {node.kind}")


def summary_nodes_to_sidebar_items(nodes: List[SummaryNode]) -> List[SidebarItem]:
    return [_node_to_sidebar_item(n) for n in nodes]

