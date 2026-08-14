#!/usr/bin/env python3
"""Restructure 02-async_tokio and 03-rust_network_programming like 01-atomic Ch01."""
from __future__ import annotations

import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 05-Async-Concurrency-Network
ASYNC = ROOT / "02-async_tokio"
NETWORK = ROOT / "03-rust_network_programming"

SECTION_MD = re.compile(r"^(\d+\.\d+(?:\.\d+)?)-.+\.md$")


def is_section_md(name: str) -> bool:
    if name in ("本章学习笔记.md", "README.md"):
        return False
    return bool(SECTION_MD.match(name))


def slug_from_md(name: str) -> str:
    return name.removesuffix(".md")


def move_section(ch: Path, slug: str) -> None:
    src_md = ch / f"{slug}.md"
    dst_md = ch / slug / f"{slug}.md"
    if src_md.is_file() and not dst_md.exists():
        dst_md.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src_md), str(dst_md))

    sec = ch / slug
    if not sec.is_dir():
        return
    code = sec / "code"
    for rs in list(sec.glob("*.rs")):
        code.mkdir(parents=True, exist_ok=True)
        dst = code / rs.name
        if not dst.exists():
            shutil.move(str(rs), str(dst))


def discover_slugs(ch: Path) -> list[str]:
    slugs: set[str] = set()
    for p in ch.glob("*.md"):
        if is_section_md(p.name):
            slugs.add(slug_from_md(p.name))
    for p in ch.iterdir():
        if p.is_dir() and SECTION_MD.match(p.name + ".md"):
            slugs.add(p.name)
    return sorted(slugs, key=lambda s: [int(x) for x in re.findall(r"\d+", s)])


def patch_cargo_toml(stage: Path) -> None:
    cargo = stage / "Cargo.toml"
    if not cargo.is_file():
        return
    t = cargo.read_text(encoding="utf-8")
    orig = t
    for slug in discover_slugs(stage):
        # path = "3.1-foo/3.1-foo-demo.rs" -> path = "3.1-foo/code/3.1-foo-demo.rs"
        t = re.sub(
            rf'(path\s*=\s*"{re.escape(slug)}/)(?!code/)([^"]+\.rs")',
            rf'\1code/\2',
            t,
        )
    if t != orig:
        cargo.write_text(t, encoding="utf-8", newline="\n")
        print(f"  patched {cargo.relative_to(ROOT)}")


def section_slugs_in(ch: Path) -> set[str]:
    return {p.name for p in ch.iterdir() if p.is_dir() and re.match(r"\d+\.\d+", p.name)}


def build_local_map(slugs: list[str]) -> dict[str, str]:
    m: dict[str, str] = {}
    for slug in slugs:
        md = f"{slug}.md"
        m[f"./{md}"] = f"./{slug}/{md}"
        m[f"](../{slug}.md)"] = f"](../{slug}/{md})"
        # demo dir
        m[f"](./{slug}/)"] = f"](./{slug}/code/)"
        m[f"[{slug}/](./{slug}/)"] = f"[{slug}/code/](./{slug}/code/)"
        # demo rs in section (old path)
        m[f"](./{slug}/"] = f"](./{slug}/code/"  # careful - might over-replace
    return m


def fix_in_section(content: str, section: str, all_sections: set[str]) -> str:
    content = content.replace(f"./{section}/", "./")
    for other in all_sections:
        if other == section:
            continue
        content = content.replace(f"](./{other}/", f"](../{other}/")
    content = content.replace("[本章学习笔记.md](./本章学习笔记.md)", "[本章学习笔记.md](../本章学习笔记.md)")
    return content


def patch_md_tree(container: Path, slugs: list[str]) -> None:
    local = build_local_map(slugs)
    sections = section_slugs_in(container)
    for md in container.rglob("*.md"):
        if "notes" in md.parts and container.name.startswith("stage"):
            # patch links inside notes/ too but skip heavy rewrites
            pass
        t = md.read_text(encoding="utf-8")
        orig = t
        rel = md.relative_to(container)
        in_sec = len(rel.parts) >= 2 and rel.parts[0] in sections
        for old, new in sorted(local.items(), key=lambda x: -len(x[0])):
            if old.endswith("/"):
                continue
            t = t.replace(old, new)
        # demo file links: ./slug/foo-demo.rs -> ./code/foo-demo.rs inside section
        if in_sec:
            sec = rel.parts[0]
            t = re.sub(rf"\(\./{re.escape(sec)}/([^)]+\.rs)\)", r"(./code/\1)", t)
            t = re.sub(rf"\(\./{re.escape(sec)}/", r"(./", t)
            t = fix_in_section(t, sec, sections)
        # chapter index demo columns
        for slug in slugs:
            t = t.replace(f"](./{slug}/)", f"](./{slug}/code/)")
            t = t.replace(f"| [{slug}/](./{slug}/code/) |", f"| [{slug}/code/](./{slug}/code/) |")
        if t != orig:
            md.write_text(t, encoding="utf-8", newline="\n")


def rewrite_chapter_index(container: Path, slugs: list[str]) -> None:
    idx = container / "本章学习笔记.md"
    if not idx.is_file():
        return
    t = idx.read_text(encoding="utf-8")
    t = t.replace("书 § = 文件名前缀", "书 § = 文件夹名")
    t = t.replace("书 § = 文件名前缀 = demo 目录", "书 § = 文件夹名")
    t = t.replace("书 § = 文件名前缀 = 代码目录名", "书 § = 文件夹名")
    for slug in slugs:
        md = f"{slug}.md"
        t = t.replace(f"](./{md})", f"](./{slug}/{md})")
        t = t.replace(f"](./{slug}/)", f"](./{slug}/code/)")
    idx.write_text(t, encoding="utf-8", newline="\n")


def restructure_container(container: Path) -> None:
    slugs = discover_slugs(container)
    if not slugs:
        return
    print(f"  {container.name}: {len(slugs)} sections")
    for slug in slugs:
        move_section(container, slug)
    patch_cargo_toml(container)
    rewrite_chapter_index(container, slugs)
    patch_md_tree(container, slugs)


def patch_async_table() -> None:
    table = ASYNC / "章节与小节对照表.md"
    if not table.is_file():
        return
    t = table.read_text(encoding="utf-8")
    for ch in sorted(ASYNC.glob("ch*")):
        if not ch.is_dir():
            continue
        ch_slug = ch.name
        for slug in discover_slugs(ch):
            md = f"{slug}.md"
            t = t.replace(
                f"](./{ch_slug}/{md})",
                f"](./{ch_slug}/{slug}/{md})",
            )
            t = t.replace(
                f"](./{ch_slug}/{slug}/)",
                f"](./{ch_slug}/{slug}/code/)",
            )
            t = t.replace(
                f"[{slug}/](./{ch_slug}/{slug}/code/)",
                f"[{slug}/code/](./{ch_slug}/{slug}/code/)",
            )
    table.write_text(t, encoding="utf-8", newline="\n")
    print("  patched async 章节与小节对照表.md")


def patch_network_spec() -> None:
    spec = NETWORK / "小节笔记与Demo规范.md"
    if not spec.is_file():
        return
    t = spec.read_text(encoding="utf-8")
    old = """| `X.Y-english-slug.md` | **该节正文**（标题用 `## X.Y …`，与书编号一致） |
| `X.Y-english-slug/*-demo.rs` | 可运行示例；由本章 `Cargo.toml` 的 `[[bin]]` 挂接 |"""
    new = """| `X.Y-english-slug/X.Y-english-slug.md` | **该节正文**（标题用 `## X.Y …`，与书编号一致） |
| `X.Y-english-slug/code/*-demo.rs` | 可运行示例；由本章 `Cargo.toml` 的 `[[bin]]` 挂接 |"""
    t = t.replace(old, new)
    t = t.replace(
        "├── 6.1-introducing-hyper.md\n├── 6.1-introducing-hyper/\n│   └── 6.1-introducing-hyper-raw-tcp-demo.rs\n├── 6.3-introducing-reqwest.md\n├── 6.3-introducing-reqwest/\n│   └── 6.3-introducing-reqwest-get-demo.rs",
        "├── 6.1-introducing-hyper/\n│   ├── 6.1-introducing-hyper.md\n│   └── code/\n│       └── 6.1-introducing-hyper-raw-tcp-demo.rs\n├── 6.3-introducing-reqwest/\n│   ├── 6.3-introducing-reqwest.md\n│   └── code/\n│       └── 6.3-introducing-reqwest-get-demo.rs",
    )
    spec.write_text(t, encoding="utf-8", newline="\n")
    print("  patched network 小节笔记与Demo规范.md")


def patch_async_readme() -> None:
    readme = ASYNC / "README.md"
    t = readme.read_text(encoding="utf-8")
    t = t.replace(
        "> **`X.Y-slug.md`** = 该节**完整精读**",
        "> **`X.Y-slug/X.Y-slug.md`** = 该节**完整精读**",
    )
    t = t.replace(
        "├── 11.3-testing-for-deadlocks.md  ← 精读正文\n└── 11.3-testing-for-deadlocks/\n    └── 11.3-testing-for-deadlocks-timeout-demo.rs",
        "└── 11.3-testing-for-deadlocks/\n    ├── 11.3-testing-for-deadlocks.md  ← 精读正文\n    └── code/\n        └── 11.3-testing-for-deadlocks-timeout-demo.rs",
    )
    readme.write_text(t, encoding="utf-8", newline="\n")


def patch_atomic_spec_async_section() -> None:
    spec = ROOT / "01-atomic" / "小节笔记与Demo规范.md"
    t = spec.read_text(encoding="utf-8")
    old = """| `X.Y-slug.md` | **该节完整精读** |
| `X.Y-slug/*-demo.rs` | **每节至少一个**可运行示例 |"""
    new = """| `X.Y-slug/X.Y-slug.md` | **该节完整精读** |
| `X.Y-slug/code/*-demo.rs` | **每节至少一个**可运行示例 |"""
    if old in t:
        t = t.replace(old, new)
        spec.write_text(t, encoding="utf-8", newline="\n")


def main() -> None:
    print("=== 02-async_tokio ===")
    for ch in sorted(ASYNC.glob("ch*")):
        if ch.is_dir():
            restructure_container(ch)

    print("=== 03-rust_network_programming (stage03–09) ===")
    for st in sorted(NETWORK.glob("stage*")):
        if st.is_dir() and (st / "Cargo.toml").is_file():
            restructure_container(st)

    print("=== meta docs ===")
    patch_async_table()
    patch_async_readme()
    patch_network_spec()
    patch_atomic_spec_async_section()

    print("restructure 02-03 done")


if __name__ == "__main__":
    main()
