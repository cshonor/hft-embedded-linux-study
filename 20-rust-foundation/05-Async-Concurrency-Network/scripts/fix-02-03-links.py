#!/usr/bin/env python3
"""Fix cross-stage and notes/ links after 02-03 restructure."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NETWORK = ROOT / "03-rust_network_programming"
ASYNC = ROOT / "02-async_tokio"


def slug_folder_path(stage: str, slug: str) -> str:
    return f"{stage}/{slug}/{slug}.md"


def fix_network_cross_links() -> None:
    # Build map from flat path to folder path for all stages
    replacements: dict[str, str] = {}
    for st in NETWORK.glob("stage*"):
        if not st.is_dir():
            continue
        stage_name = st.name
        for sec in st.iterdir():
            if sec.is_dir() and re.match(r"\d+\.\d+", sec.name):
                slug = sec.name
                flat = f"{stage_name}/{slug}.md"
                nested = slug_folder_path(stage_name, slug)
                replacements[flat] = nested
                replacements[f"../{flat}"] = f"../{nested}"
                replacements[f"../../{flat}"] = f"../../{nested}"

    for md in NETWORK.rglob("*.md"):
        t = md.read_text(encoding="utf-8")
        orig = t
        for old, new in sorted(replacements.items(), key=lambda x: -len(x[0])):
            t = t.replace(old, new)
        # notes/ from inside section folder
        rel = md.relative_to(NETWORK)
        if len(rel.parts) >= 2 and rel.parts[0].startswith("stage") and re.match(
            r"\d+\.\d+", rel.parts[1]
        ):
            t = t.replace("](./notes/", "](../notes/")
            t = t.replace("](notes/", "](../notes/")
        if t != orig:
            md.write_text(t, encoding="utf-8", newline="\n")
            print("fixed", md.relative_to(ROOT))


def fix_async_cross_links() -> None:
    replacements: dict[str, str] = {}
    for ch in ASYNC.glob("ch*"):
        if not ch.is_dir():
            continue
        ch_name = ch.name
        for sec in ch.iterdir():
            if sec.is_dir() and re.match(r"\d+\.\d+", sec.name):
                slug = sec.name
                flat = f"{ch_name}/{slug}.md"
                nested = f"{ch_name}/{slug}/{slug}.md"
                replacements[flat] = nested
                replacements[f"../{flat}"] = f"../{nested}"

    for md in ASYNC.rglob("*.md"):
        t = md.read_text(encoding="utf-8")
        orig = t
        for old, new in sorted(replacements.items(), key=lambda x: -len(x[0])):
            t = t.replace(old, new)
        if t != orig:
            md.write_text(t, encoding="utf-8", newline="\n")
            print("fixed", md.relative_to(ROOT))


def patch_network_readme() -> None:
    readme = NETWORK / "README.md"
    t = readme.read_text(encoding="utf-8")
    t = t.replace(
        "**笔记 + Demo 约定**（与 `01-atomic/` 相同）",
        "**笔记 + Demo 约定**（与 `01-atomic/` 相同：`X.Y-slug/X.Y-slug.md` + `code/`）",
    )
    readme.write_text(t, encoding="utf-8", newline="\n")


def main() -> None:
    fix_network_cross_links()
    fix_async_cross_links()
    patch_network_readme()
    print("link fixes done")


if __name__ == "__main__":
    main()
