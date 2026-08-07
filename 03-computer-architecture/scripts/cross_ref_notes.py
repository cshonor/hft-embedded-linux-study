#!/usr/bin/env python3
"""03-computer-architecture <-> 02/00/09 笔记级互链脚本。

在 03 的 🔴 章节 (Ch2/Ch5) 笔记中添加指向 02 CSAPP / 00 Harris 的 note-level 链接,
同时在 02/00 对应笔记中添加反向 Hennessy 链接。

格式:
  03 侧: 标题行后加 `> ↔ [CSAPP §X.Y name](path) · [Harris §X.Y name](path)`
  02 侧: 已有 `> ↔ [Harris ...]` 行后加 `> ↔ [Hennessy §X.Y name](path)`
  00 侧: `> **Link Target:**` 行追加 `· ↔ [Hennessy §X.Y name](path)`
"""

import os, re

# script is at hft/03-computer-architecture/scripts/cross_ref_notes.py
# need to go up 2 levels to reach hft root
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))      # .../03-computer-architecture/scripts
MOD03 = os.path.dirname(SCRIPT_DIR)                           # .../03-computer-architecture
BASE = os.path.dirname(MOD03)                                  # .../hft root

def p(*parts):
    return os.path.join(BASE, *parts)

# ── 03 侧: (03_note_filename, chapter_dir, links_to_add)
# links_to_add = list of (label, relative_path_from_03_note)
ch2_dir = "chapter-02-memory-hierarchy-design"
ch5_dir = "chapter-05-thread-level-parallelism"

# Relative path from 03 note to target
def to_02(ch, note):
    return f"../../../02-computer-systems/{ch}/notes/{note}"

def to_00(ch_dir, note):
    return f"../../../00-digital-logic-cpu/{ch_dir}/{note}"

CROSS_REFS_03 = [
    # ── Ch2 ↔ 02 CSAPP Ch6 ──
    {
        "file": f"{ch2_dir}/notes/section-2.1-引言与存储器层次.md",
        "links": [
            ("CSAPP §6.3 层次结构", to_02("chapter-06-memory-hierarchy", "section-6.3-层次结构与缓存概念.md")),
            ("Harris §8.3 高速缓存", to_00("ch08_memory", "8.3_高速缓存.md")),
        ],
    },
    {
        "file": f"{ch2_dir}/notes/section-2.2-存储器技术与优化.md",
        "links": [
            ("CSAPP §6.1 存储技术", to_02("chapter-06-memory-hierarchy", "section-6.1-存储技术.md")),
        ],
    },
    {
        "file": f"{ch2_dir}/notes/section-2.3-缓存性能十项高级优化.md",
        "links": [
            ("CSAPP §6.4.7 Cache参数影响", to_02("chapter-06-memory-hierarchy", "section-6.4.7-Cache参数的性能影响.md")),
            ("CSAPP §6.5 缓存友好代码", to_02("chapter-06-memory-hierarchy", "section-6.5-编写高速缓存友好的代码.md")),
            ("Harris §8.2 存储器性能分析", to_00("ch08_memory", "8.2_存储器系统性能分析.md")),
        ],
    },
    {
        "file": f"{ch2_dir}/notes/section-2.4-虚拟内存与虚拟机.md",
        "links": [
            ("CSAPP §9.3 VM作为缓存", to_02("chapter-09-virtual-memory", "section-9.3-虚拟内存作为缓存工具.md")),
            ("CSAPP §9.6 地址翻译", to_02("chapter-09-virtual-memory", "section-9.6-地址翻译.md")),
            ("Harris §8.4 虚拟存储器", to_00("ch08_memory", "8.4_虚拟存储器.md")),
        ],
    },
    {
        "file": f"{ch2_dir}/notes/section-2.7-谬误与陷阱.md",
        "links": [
            ("CSAPP §6.7 小结", to_02("chapter-06-memory-hierarchy", "section-6.7-小结.md")),
        ],
    },
    # ── Ch5 ↔ 02 CSAPP Ch12 ──
    {
        "file": f"{ch5_dir}/notes/section-5.1-引言与多处理器挑战.md",
        "links": [
            ("CSAPP §12.6 线程并行性", to_02("chapter-12-concurrent-programming", "section-12.6-使用线程提高并行性.md")),
            ("Harris §7.7 高级微结构", to_00("ch07_microarchitecture", "7.7_高级微结构.md")),
        ],
    },
    {
        "file": f"{ch5_dir}/notes/section-5.3-性能分析与伪共享.md",
        "links": [
            ("CSAPP §12.4 共享变量", to_02("chapter-12-concurrent-programming", "section-12.4-多线程程序中的共享变量.md")),
            ("Harris §8.2 存储器性能分析", to_00("ch08_memory", "8.2_存储器系统性能分析.md")),
        ],
    },
    {
        "file": f"{ch5_dir}/notes/section-5.5-同步基础.md",
        "links": [
            ("CSAPP §12.5 信号量", to_02("chapter-12-concurrent-programming", "section-12.5-信号量与预线程化.md")),
        ],
    },
    {
        "file": f"{ch5_dir}/notes/section-5.6-内存一致性模型.md",
        "links": [
            ("CSAPP §12.7 并发问题", to_02("chapter-12-concurrent-programming", "section-12.7-其他并发问题.md")),
        ],
    },
    {
        "file": f"{ch5_dir}/notes/section-5.7-5.11-交叉问题实例与展望.md",
        "links": [
            ("CSAPP §12.8 小结", to_02("chapter-12-concurrent-programming", "section-12.8-小结.md")),
        ],
    },
]

# ── 02 侧反向链接: (02_note_path, 03_chapter_dir, 03_note_filename, label)
CROSS_REFS_02 = [
    # 02 Ch6 -> 03 Ch2
    ("02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.1-存储技术.md",
     "chapter-02-memory-hierarchy-design", "section-2.2-存储器技术与优化.md", "Hennessy §2.2 存储器技术"),
    ("02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.3-层次结构与缓存概念.md",
     "chapter-02-memory-hierarchy-design", "section-2.1-引言与存储器层次.md", "Hennessy §2.1 存储器层次"),
    ("02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.4.7-Cache参数的性能影响.md",
     "chapter-02-memory-hierarchy-design", "section-2.3-缓存性能十项高级优化.md", "Hennessy §2.3 缓存优化"),
    ("02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.5-编写高速缓存友好的代码.md",
     "chapter-02-memory-hierarchy-design", "section-2.3-缓存性能十项高级优化.md", "Hennessy §2.3 缓存优化"),
    ("02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.7-小结.md",
     "chapter-02-memory-hierarchy-design", "section-2.7-谬误与陷阱.md", "Hennessy §2.7 谬误与陷阱"),
    # 02 Ch9 -> 03 Ch2
    ("02-computer-systems/chapter-09-virtual-memory/notes/section-9.3-虚拟内存作为缓存工具.md",
     "chapter-02-memory-hierarchy-design", "section-2.4-虚拟内存与虚拟机.md", "Hennessy §2.4 虚拟内存"),
    ("02-computer-systems/chapter-09-virtual-memory/notes/section-9.6-地址翻译.md",
     "chapter-02-memory-hierarchy-design", "section-2.4-虚拟内存与虚拟机.md", "Hennessy §2.4 虚拟内存"),
    # 02 Ch12 -> 03 Ch5
    ("02-computer-systems/chapter-12-concurrent-programming/notes/section-12.4-多线程程序中的共享变量.md",
     "chapter-05-thread-level-parallelism", "section-5.3-性能分析与伪共享.md", "Hennessy §5.3 伪共享"),
    ("02-computer-systems/chapter-12-concurrent-programming/notes/section-12.5-信号量与预线程化.md",
     "chapter-05-thread-level-parallelism", "section-5.5-同步基础.md", "Hennessy §5.5 同步基础"),
    ("02-computer-systems/chapter-12-concurrent-programming/notes/section-12.6-使用线程提高并行性.md",
     "chapter-05-thread-level-parallelism", "section-5.1-引言与多处理器挑战.md", "Hennessy §5.1 TLP"),
    ("02-computer-systems/chapter-12-concurrent-programming/notes/section-12.7-其他并发问题.md",
     "chapter-05-thread-level-parallelism", "section-5.6-内存一致性模型.md", "Hennessy §5.6 一致性模型"),
    ("02-computer-systems/chapter-12-concurrent-programming/notes/section-12.8-小结.md",
     "chapter-05-thread-level-parallelism", "section-5.7-5.11-交叉问题实例与展望.md", "Hennessy §5.7 交叉问题"),
]

# ── 00 侧反向链接: (00_note_path, 03_chapter_dir, 03_note_filename, label)
CROSS_REFS_00 = [
    ("00-digital-logic-cpu/ch08_memory/8.2_存储器系统性能分析.md",
     "chapter-02-memory-hierarchy-design", "section-2.3-缓存性能十项高级优化.md", "Hennessy §2.3 缓存优化"),
    ("00-digital-logic-cpu/ch08_memory/8.2_存储器系统性能分析.md",
     "chapter-05-thread-level-parallelism", "section-5.3-性能分析与伪共享.md", "Hennessy §5.3 伪共享"),
    ("00-digital-logic-cpu/ch08_memory/8.3_高速缓存.md",
     "chapter-02-memory-hierarchy-design", "section-2.1-引言与存储器层次.md", "Hennessy §2.1 存储器层次"),
    ("00-digital-logic-cpu/ch08_memory/8.4_虚拟存储器.md",
     "chapter-02-memory-hierarchy-design", "section-2.4-虚拟内存与虚拟机.md", "Hennessy §2.4 虚拟内存"),
    ("00-digital-logic-cpu/ch07_microarchitecture/7.7_高级微结构.md",
     "chapter-05-thread-level-parallelism", "section-5.1-引言与多处理器挑战.md", "Hennessy §5.1 TLP"),
]

def add_03_links():
    """Add cross-reference links to 03 notes (after the ## header line)."""
    ok, miss = 0, 0
    for entry in CROSS_REFS_03:
        fpath = os.path.join(MOD03, entry["file"])
        if not os.path.exists(fpath):
            print(f"MISSING: {fpath}")
            miss += 1
            continue
        with open(fpath, "r", encoding="utf-8") as f:
            content = f.read()

        # Skip if already has ↔ link
        if "↔ [CSAPP" in content or "↔ [Harris" in content:
            print(f"SKIP (already has links): {entry['file']}")
            ok += 1
            continue

        # Build the link line
        link_strs = []
        for label, rel_path in entry["links"]:
            link_strs.append(f"[{label}]({rel_path})")
        link_line = "> ↔ " + " · ".join(link_strs) + "\n"

        # Insert after the first line (## header)
        lines = content.split("\n")
        # Find the first non-empty line after the header
        insert_idx = 1
        for i in range(1, min(4, len(lines))):
            if lines[i].strip() == "":
                insert_idx = i + 1
                break
            else:
                insert_idx = i + 1
                break

        # Insert link line + blank line after the header
        lines.insert(insert_idx, "")
        lines.insert(insert_idx + 1, link_line.rstrip())
        lines.insert(insert_idx + 2, "")

        with open(fpath, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        print(f"OK: {entry['file']}")
        ok += 1

    print(f"\n03 side: {ok} OK, {miss} MISSING")
    return ok, miss

def add_02_links():
    """Add Hennessy backlinks to 02 CSAPP notes."""
    ok, miss = 0, 0
    for note_path, ch3_dir, note3_file, label in CROSS_REFS_02:
        fpath = p(note_path)
        if not os.path.exists(fpath):
            print(f"MISSING: {fpath}")
            miss += 1
            continue
        with open(fpath, "r", encoding="utf-8") as f:
            content = f.read()

        if "Hennessy" in content:
            print(f"SKIP (already has Hennessy): {note_path}")
            ok += 1
            continue

        # Relative path from 02 note to 03 note
        rel = f"../../../03-computer-architecture/{ch3_dir}/notes/{note3_file}"
        hennessy_line = f"> ↔ [{label}]({rel})"

        lines = content.split("\n")

        # Find existing ↔ line (Harris link) and insert after it
        found = False
        for i, line in enumerate(lines):
            if line.startswith("> ↔ [Harris"):
                lines.insert(i + 1, hennessy_line)
                found = True
                break

        if not found:
            # Find the first ## header and insert after it
            for i, line in enumerate(lines):
                if line.startswith("## "):
                    lines.insert(i + 1, "")
                    lines.insert(i + 2, hennessy_line)
                    lines.insert(i + 3, "")
                    found = True
                    break

        if not found:
            print(f"SKIP (no insertion point): {note_path}")
            miss += 1
            continue

        with open(fpath, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        print(f"OK: {note_path}")
        ok += 1

    print(f"\n02 side: {ok} OK, {miss} MISSING")
    return ok, miss

def add_00_links():
    """Add Hennessy backlinks to 00 Harris notes (append to Link Target line)."""
    ok, miss = 0, 0
    for note_path, ch3_dir, note3_file, label in CROSS_REFS_00:
        fpath = p(note_path)
        if not os.path.exists(fpath):
            print(f"MISSING: {fpath}")
            miss += 1
            continue
        with open(fpath, "r", encoding="utf-8") as f:
            content = f.read()

        if "Hennessy" in content:
            print(f"SKIP (already has Hennessy): {note_path}")
            ok += 1
            continue

        # Relative path from 00 note to 03 note
        rel = f"../../../03-computer-architecture/{ch3_dir}/notes/{note3_file}"

        # Find the Link Target line and append
        lines = content.split("\n")
        found = False
        for i, line in enumerate(lines):
            if "**Link Target:**" in line:
                # Append to the end of this line
                if line.endswith("  "):
                    lines[i] = line.rstrip() + f" · ↔ [{label}]({rel})  "
                else:
                    lines[i] = line + f" · ↔ [{label}]({rel})"
                found = True
                break

        if not found:
            print(f"SKIP (no Link Target): {note_path}")
            miss += 1
            continue

        with open(fpath, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        print(f"OK: {note_path}")
        ok += 1

    print(f"\n00 side: {ok} OK, {miss} MISSING")
    return ok, miss

if __name__ == "__main__":
    print("=" * 60)
    print("03 ↔ 02/00 笔记级互链")
    print("=" * 60)
    print("\n--- 03 侧添加链接 ---")
    add_03_links()
    print("\n--- 02 侧添加反向链接 ---")
    add_02_links()
    print("\n--- 00 侧添加反向链接 ---")
    add_00_links()
    print("\nDone!")
