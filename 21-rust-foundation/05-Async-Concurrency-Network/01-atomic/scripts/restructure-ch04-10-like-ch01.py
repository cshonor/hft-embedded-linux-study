#!/usr/bin/env python3
"""Restructure Chapter-04 … Chapter-10 to match Chapter-01/02/03 layout."""
from __future__ import annotations

import re
import shutil
from pathlib import Path

ATOMIC = Path(__file__).resolve().parents[1]
MANIFEST = "05-Async-Concurrency-Network/01-atomic/Cargo.toml"


def slug_from_name(name: str) -> str:
    """4.1-minimal-implementation.md -> 4.1-minimal-implementation"""
    return name.removesuffix(".md")


def move_root_md(ch: Path, slug: str) -> None:
    src = ch / f"{slug}.md"
    dst = ch / slug / f"{slug}.md"
    if src.is_file() and not dst.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src), str(dst))


def move_sub_md(ch: Path, section: str, name: str) -> None:
    src = ch / name
    dst = ch / section / name
    if src.is_file() and not dst.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src), str(dst))


def move_demos(ch: Path, section: str, demos: list[str]) -> None:
    code_dir = ch / section / "code"
    for demo in demos:
        for src in [ch / section / demo, ch / demo]:
            if src.is_file():
                code_dir.mkdir(parents=True, exist_ok=True)
                dst = code_dir / demo
                if not dst.exists():
                    shutil.move(str(src), str(dst))


def glob_demos(ch: Path, section: str) -> list[str]:
    sec = ch / section
    if not sec.is_dir():
        return []
    return [p.name for p in sec.glob("*.rs")]


def restructure_chapter(ch_name: str, sections: list[dict]) -> None:
    ch = ATOMIC / ch_name
    if not ch.is_dir():
        print(f"skip missing {ch_name}")
        return

    for sec in sections:
        slug = sec["slug"]
        demos = sec.get("demos")
        if demos is None:
            demos = glob_demos(ch, slug)
        move_root_md(ch, slug)
        for sub in sec.get("subs", []):
            move_sub_md(ch, slug, sub)
        move_demos(ch, slug, demos)

    print(f"  moved files in {ch_name}")


# --- chapter definitions ---

CH04 = [
    {"slug": "4.1-minimal-implementation", "demos": ["4.1-minimal-implementation-demo.rs"]},
    {"slug": "4.2-unsafe-spin-lock"},
    {"slug": "4.3-safe-lock-guard"},
    {"slug": "4.4-summary"},
]

CH05 = [
    {"slug": "5.1-mutex-based-channel", "demos": ["5.1-mutex-based-channel-demo.rs"]},
    {"slug": "5.2-unsafe-one-shot-channel", "demos": ["5.2-unsafe-one-shot-channel-demo.rs"]},
    {"slug": "5.3-runtime-checks-safety"},
    {"slug": "5.4-types-safety"},
    {"slug": "5.5-borrowing-avoid-allocation"},
    {"slug": "5.6-blocking"},
    {"slug": "5.7-summary"},
]

CH06 = [
    {"slug": "6.1-basic-reference-counting", "demos": ["6.1-basic-reference-counting-demo.rs"]},
    {"slug": "6.2-testing-it", "demos": ["6.2-testing-it-demo.rs"]},
    {"slug": "6.3-mutation", "demos": ["6.3-mutation-demo.rs"]},
    {"slug": "6.4-weak-pointers", "demos": ["6.4-weak-pointers-demo.rs"]},
    {"slug": "6.5-testing-weak"},
    {"slug": "6.6-optimizing", "demos": ["6.6-optimizing-align-demo.rs"]},
    {"slug": "6.7-summary"},
]

CH07 = [
    {"slug": "7.1-processor-instructions"},
    {"slug": "7.2-caching", "demos": ["7.2-caching-false-sharing-demo.rs"]},
    {"slug": "7.3-reordering", "demos": ["7.3-reordering-relaxed-demo.rs"]},
    {"slug": "7.4-memory-fences"},
    {"slug": "7.5-summary"},
]

CH08 = [
    {
        "slug": "8.1-futex",
        "subs": [
            "8.1.1-futex-two-phase.md",
            "8.1.2-futex-wait.md",
            "8.1.3-futex-wake.md",
            "8.1.4-futex-requeue-shared.md",
        ],
    },
    {
        "slug": "8.2-thread-parks",
        "subs": ["8.2.1-park-unpark-semantics.md", "8.2.2-park-vs-futex-condvar.md"],
    },
    {
        "slug": "8.3-condition-variables",
        "subs": [
            "8.3.1-pthread-condvar.md",
            "8.3.2-windows-condvar.md",
            "8.3.3-rust-os-mapping.md",
        ],
    },
    {"slug": "8.4-summary"},
]

CH09 = [
    {
        "slug": "9.1-mutex",
        "subs": [
            "9.1.1-mutex-baseline.md",
            "9.1.2-three-state-avoid-syscall.md",
            "9.1.3-hybrid-lock.md",
            "9.1.4-false-sharing-benchmarks.md",
        ],
    },
    {
        "slug": "9.2-condition-variable",
        "subs": ["9.2.1-condvar-avoid-syscall.md", "9.2.2-spurious-wakeup-notify.md"],
    },
    {
        "slug": "9.3-reader-writer-lock",
        "subs": [
            "9.3.1-writer-busy-loop.md",
            "9.3.2-writer-starvation-state-machine.md",
            "9.3.3-vs-std-rwlock.md",
        ],
    },
    {"slug": "9.4-summary"},
]

CH10 = [
    {
        "slug": "10.1-semaphores",
        "subs": ["10.1.1-semaphore-basics.md", "10.1.2-binary-vs-counting.md"],
        "demos": ["10.1-semaphores-demo.rs"],
    },
    {
        "slug": "10.2-rcu",
        "subs": ["10.2.1-rcu-pattern.md", "10.2.2-rcu-reclamation.md"],
        "demos": ["10.2-rcu-pointer-swap-demo.rs"],
    },
    {
        "slug": "10.3-lock-free-linked-list",
        "subs": ["10.3.1-lock-free-stack-push.md", "10.3.2-aba-recycling.md"],
        "demos": ["10.3-lock-free-stack-push-demo.rs"],
    },
    {"slug": "10.4-queue-based-locks"},
    {"slug": "10.5-blocking-locks"},
    {"slug": "10.6-seqlocks"},
    {
        "slug": "10.7-teaching-materials",
        "subs": [
            "10.7.1-design-top-down.md",
            "10.7.2-performance-practices.md",
            "10.7.3-debug-pitfalls.md",
            "10.7.4-interview-cheatsheet.md",
        ],
    },
]

CHAPTER_SECTIONS = {
    "Chapter-04-Spin-Locks": CH04,
    "Chapter-05-Channels": CH05,
    "Chapter-06-Custom-Arc": CH06,
    "Chapter-07-Processors": CH07,
    "Chapter-08-OS-Primitives": CH08,
    "Chapter-09-Custom-Locks": CH09,
    "Chapter-10-Advanced-Concurrent-Data-Structures": CH10,
}


def write_mod_rs(ch_name: str, content: str) -> None:
    (ATOMIC / ch_name / "mod.rs").write_text(content.strip() + "\n", encoding="utf-8")


def write_readme(ch_name: str, title: str, section_count: int, prev_ch: str | None, next_ch: str | None) -> None:
    prev_line = f"- **上一章**：[{prev_ch}](../{prev_ch}/本章学习笔记.md)  \n" if prev_ch else ""
    next_line = f"- **下一章**：[{next_ch}](../{next_ch}/本章学习笔记.md)  \n" if next_ch else ""
    text = f"""# {title}

**唯一规则（与第一～三章、实体书一致）**

| 书 § | 笔记 | 代码（有 demo 时） |
|------|------|-------------------|
| X.Y | `X.Y-english-slug/X.Y-english-slug.md` + 子笔记 `X.Y.Z-*.md` | `X.Y-english-slug/code/*.rs` |

- **章入口**：[本章学习笔记.md](./本章学习笔记.md)（索引表，共 {section_count} 节）  
{prev_line}{next_line}- 运行：`cargo build --manifest-path {MANIFEST}`"""
    (ATOMIC / ch_name / "README.md").write_text(text.strip() + "\n", encoding="utf-8")


def demo_cell(ch_name: str, slug: str, has_demo: bool) -> str:
    if not has_demo:
        return "—"
    return f"[{slug}/code/](./{slug}/code/)"


def write_simple_index(ch_name: str, chapter_num: int, sections: list[dict]) -> None:
    rows = []
    for sec in sections:
        slug = sec["slug"]
        md = f"{slug}.md"
        has_demo = bool(sec.get("demos") or glob_demos(ATOMIC / ch_name, slug))
        demo = demo_cell(ch_name, slug, has_demo)
        rows.append(f"| {slug.split('-')[0]} | [{md}](./{slug}/{md}) | {demo} |")
        for sub in sec.get("subs", []):
            sub_slug = slug_from_name(sub)
            num = sub_slug.rsplit("-", 1)[0] if "." in sub_slug else sub_slug
            # extract section number like 8.1.1 from filename
            m = re.match(r"(\d+\.\d+\.\d+)", sub_slug)
            label = m.group(1) if m else sub_slug
            rows.append(f"| {label} | [{sub}](./{slug}/{sub}) | {'↑' if has_demo else '—'} |")

    text = f"""# 第{chapter_num}章 — 学习笔记索引

> **结构**：书 § = 文件夹名；每节含 **索引**、子笔记、`code/` 下 demo（如有）。书目：[全书目录-与实体书一致.md](../全书目录-与实体书一致.md)

| 书 § | 笔记 | Demo |
|------|------|------|
{chr(10).join(rows)}

```bash
cargo build --manifest-path {MANIFEST}
```"""
    (ATOMIC / ch_name / "本章学习笔记.md").write_text(text.strip() + "\n", encoding="utf-8")


def patch_index_table(ch_name: str, sections: list[dict]) -> None:
    """Patch paths in rich chapter indexes (8/9/10) without dropping prose."""
    idx = ATOMIC / ch_name / "本章学习笔记.md"
    if not idx.is_file():
        write_simple_index(ch_name, int(ch_name.split("-")[0].removeprefix("Chapter")), sections)
        return
    t = idx.read_text(encoding="utf-8")
    t = t.replace(
        "> **结构**：书 § = 文件名前缀",
        "> **结构**：书 § = 文件夹名",
    )
    t = t.replace(
        "> **结构**：书 § = 文件名前缀 = 代码目录名",
        "> **结构**：书 § = 文件夹名",
    )
    for sec in sections:
        slug = sec["slug"]
        md = f"{slug}.md"
        has_demo = bool(sec.get("demos"))
        # index md link
        for old in [
            f"](./{md})",
            f"](({md})",
            f"](./{slug}.md)",
        ]:
            t = t.replace(old, f"](./{slug}/{md})")
        # demo folder
        if has_demo:
            t = t.replace(
                f"[{slug}/](./{slug}/)",
                f"[{slug}/code/](./{slug}/code/)",
            )
            t = t.replace(
                f"](./{slug}/)",
                f"](./{slug}/code/)",
            )
        for sub in sec.get("subs", []):
            t = t.replace(f"](./{sub})", f"](./{slug}/{sub})")
    idx.write_text(t, encoding="utf-8")


def build_link_map(sections: list[dict]) -> dict[str, str]:
    """Old relative path fragment -> new path (within same chapter)."""
    m: dict[str, str] = {}
    for sec in sections:
        slug = sec["slug"]
        md = f"{slug}.md"
        m[f"./{md}"] = f"./{slug}/{md}"
        m[f"](../{slug}.md)"] = f"](../{slug}/{md})"
        for sub in sec.get("subs", []):
            m[f"./{sub}"] = f"./{slug}/{sub}"
        if sec.get("demos"):
            for demo in sec["demos"]:
                m[f"./{slug}/{demo}"] = f"./{slug}/code/{demo}"
                m[f"](./{slug}/)"] = f"](./{slug}/code/)"
    return m


def build_cross_chapter_map(ch_name: str, sections: list[dict]) -> dict[str, str]:
    prefix = f"{ch_name}/"
    m: dict[str, str] = {}
    for sec in sections:
        slug = sec["slug"]
        md = f"{slug}.md"
        m[f"{prefix}{md}"] = f"{prefix}{slug}/{md}"
        for sub in sec.get("subs", []):
            m[f"{prefix}{sub}"] = f"{prefix}{slug}/{sub}"
        for demo in sec.get("demos") or []:
            m[f"{prefix}{slug}/{demo}"] = f"{prefix}{slug}/code/{demo}"
    return m


def patch_md_file(path: Path, replacements: dict[str, str], in_section: bool) -> None:
    t = path.read_text(encoding="utf-8")
    orig = t
    for old, new in sorted(replacements.items(), key=lambda x: -len(x[0])):
        t = t.replace(old, new)
    if in_section:
        t = t.replace("[本章学习笔记.md](./本章学习笔记.md)", "[本章学习笔记.md](../本章学习笔记.md)")
        t = t.replace("§X 索引：[本章学习笔记.md](./本章学习笔记.md)", "§X 索引：[本章学习笔记.md](../本章学习笔记.md)")
        # section index footer variants
        t = re.sub(
            r"§[\d.]+\s*索引：\[本章学习笔记\.md\]\(\./本章学习笔记\.md\)",
            lambda m: m.group(0).replace("./本章学习笔记.md", "../本章学习笔记.md"),
            t,
        )
    t = t.replace("`atomic/Cargo.toml`", f"`{MANIFEST}`")
    if t != orig:
        path.write_text(t, encoding="utf-8", newline="\n")


def patch_chapter_links(ch_name: str, sections: list[dict]) -> None:
    ch = ATOMIC / ch_name
    local = build_link_map(sections)
    for md in ch.rglob("*.md"):
        rel_parts = md.relative_to(ch).parts
        in_section = len(rel_parts) > 1 and rel_parts[0] not in ("README.md",)
        patch_md_file(md, local, in_section and md.name != "本章学习笔记.md")


def patch_repo_cross_links(all_maps: dict[str, dict[str, str]]) -> None:
    combined: dict[str, str] = {}
    for m in all_maps.values():
        combined.update(m)
    for md in ATOMIC.rglob("*.md"):
        t = md.read_text(encoding="utf-8")
        orig = t
        for old, new in sorted(combined.items(), key=lambda x: -len(x[0])):
            t = t.replace(f"](./{old})", f"](./{new})")
            t = t.replace(f"]({old})", f"]({new})")
            # paths without ./
            bare_old = old
            bare_new = new
            t = t.replace(bare_old, bare_new)
        # Ch3 summary had old Ch04 demo path
        t = t.replace(
            "Chapter-04-Spin-Locks/4.1-minimal-implementation/4.1-minimal-implementation-demo.rs",
            "Chapter-04-Spin-Locks/4.1-minimal-implementation/code/4.1-minimal-implementation-demo.rs",
        )
        t = t.replace(
            "../Chapter-04-Spin-Locks/4.1-minimal-implementation/4.1-minimal-implementation-demo.rs",
            "../Chapter-04-Spin-Locks/4.1-minimal-implementation/code/4.1-minimal-implementation-demo.rs",
        )
        if t != orig:
            md.write_text(t, encoding="utf-8", newline="\n")


def update_mod_files() -> None:
    write_mod_rs(
        "Chapter-04-Spin-Locks",
        """//! 第 4 章 — 自旋锁（§4.1～4.3 同一实现递进）

#[path = "4.1-minimal-implementation/code/4.1-minimal-implementation-demo.rs"]
pub mod spin_lock;

pub fn demo() {
    spin_lock::demo();
}""",
    )
    write_mod_rs(
        "Chapter-05-Channels",
        """//! 第 5 章 — 通道

#[path = "5.1-mutex-based-channel/code/5.1-mutex-based-channel-demo.rs"]
pub mod mutex_channel;

#[path = "5.2-unsafe-one-shot-channel/code/5.2-unsafe-one-shot-channel-demo.rs"]
pub mod one_shot_channel;

pub fn demo() {
    mutex_channel::demo();
    one_shot_channel::demo();
}""",
    )
    write_mod_rs(
        "Chapter-06-Custom-Arc",
        """//! 第 6 章 — 自定义 Arc

#[path = "6.1-basic-reference-counting/code/6.1-basic-reference-counting-demo.rs"]
pub mod custom_arc;

#[path = "6.3-mutation/code/6.3-mutation-demo.rs"]
pub mod arc_mutex;

#[path = "6.2-testing-it/code/6.2-testing-it-demo.rs"]
pub mod testing;

#[path = "6.4-weak-pointers/code/6.4-weak-pointers-demo.rs"]
pub mod weak_arc;

#[path = "6.6-optimizing/code/6.6-optimizing-align-demo.rs"]
pub mod optimizing;

pub fn demo() {
    custom_arc::demo();
    testing::demo();
    arc_mutex::demo();
    weak_arc::demo();
    optimizing::demo();
}""",
    )
    write_mod_rs(
        "Chapter-07-Processors",
        """//! 第 7 章 — 处理器与缓存

#[path = "7.2-caching/code/7.2-caching-false-sharing-demo.rs"]
pub mod false_sharing;

#[path = "7.3-reordering/code/7.3-reordering-relaxed-demo.rs"]
pub mod reordering;

pub fn demo() {
    false_sharing::demo();
}""",
    )
    write_mod_rs(
        "Chapter-10-Advanced-Concurrent-Data-Structures",
        """//! 第 10 章 — 项目与灵感

#[path = "10.1-semaphores/code/10.1-semaphores-demo.rs"]
pub mod semaphores;

#[path = "10.2-rcu/code/10.2-rcu-pointer-swap-demo.rs"]
pub mod rcu;

#[path = "10.3-lock-free-linked-list/code/10.3-lock-free-stack-push-demo.rs"]
pub mod lock_free_stack;

pub fn demo() {
    semaphores::demo();
}""",
    )


def main() -> None:
    order = [
        ("Chapter-04-Spin-Locks", CH04, "第四章", 4, "Chapter-03-Memory-Ordering", "Chapter-05-Channels"),
        ("Chapter-05-Channels", CH05, "第五章", 5, "Chapter-04-Spin-Locks", "Chapter-06-Custom-Arc"),
        ("Chapter-06-Custom-Arc", CH06, "第六章", 6, "Chapter-05-Channels", "Chapter-07-Processors"),
        ("Chapter-07-Processors", CH07, "第七章", 7, "Chapter-06-Custom-Arc", "Chapter-08-OS-Primitives"),
        ("Chapter-08-OS-Primitives", CH08, "第八章", 8, "Chapter-07-Processors", "Chapter-09-Custom-Locks"),
        ("Chapter-09-Custom-Locks", CH09, "第九章", 9, "Chapter-08-OS-Primitives", "Chapter-10-Advanced-Concurrent-Data-Structures"),
        ("Chapter-10-Advanced-Concurrent-Data-Structures", CH10, "第十章", 10, "Chapter-09-Custom-Locks", None),
    ]

    print("=== file moves ===")
    for ch_name, sections, *_ in order:
        restructure_chapter(ch_name, sections)

    print("=== mod.rs ===")
    update_mod_files()

    print("=== README + 本章学习笔记 ===")
    for ch_name, sections, title, n, prev_ch, next_ch in order:
        write_readme(ch_name, title, len(sections), prev_ch, next_ch)
        if ch_name in (
            "Chapter-04-Spin-Locks",
            "Chapter-05-Channels",
            "Chapter-06-Custom-Arc",
            "Chapter-07-Processors",
        ):
            write_simple_index(ch_name, n, sections)
        else:
            patch_index_table(ch_name, sections)

    print("=== link patches ===")
    all_cross: dict[str, dict[str, str]] = {}
    for ch_name, sections in CHAPTER_SECTIONS.items():
        patch_chapter_links(ch_name, sections)
        all_cross[ch_name] = build_cross_chapter_map(ch_name, sections)
    patch_repo_cross_links(all_cross)

    print("ch04-10 restructure done")


if __name__ == "__main__":
    main()
