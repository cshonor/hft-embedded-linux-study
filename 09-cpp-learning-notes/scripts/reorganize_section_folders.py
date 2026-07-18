#!/usr/bin/env python3
"""Move Primer section notes into per-section folders and fix markdown links."""
from __future__ import annotations

import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRIMER = ROOT / "01-C++Primer"

# (chapter_dir_name, folder_name, [filenames relative to chapter])
SECTION_FOLDERS: list[tuple[str, str, list[str]]] = [
    (
        "ch01-getting-started",
        "1.5-a-brief-introduction-to-classes",
        [
            "1.5-类简介.md",
            "1.5.1-类的使用与头文件分离.md",
            "1.5.2-链接编译产物与示例.md",
        ],
    ),
    (
        "ch02-variables-and-basic-types",
        "2.2-variables",
        [
            "2.2-变量.md",
            "2.2.1-标识符与未初始化.md",
            "2.2.2-变量常量与static.md",
            "2.2.3-变量遮蔽.md",
            "2.2.4-extern与头文件分工.md",
        ],
    ),
    (
        "ch02-variables-and-basic-types",
        "2.3-compound-types",
        [
            "2.3-复合类型.md",
            "2.3.1-引用.md",
            "2.3.2-指针与nullptr.md",
        ],
    ),
    (
        "ch02-variables-and-basic-types",
        "2.4-const-qualifier",
        [
            "2.4-const限定符.md",
            "2.4.1-const基础与constexpr.md",
            "2.4.2-const口诀与声明语法.md",
            "2.4.3-constexpr详解.md",
        ],
    ),
    (
        "ch02-variables-and-basic-types",
        "2.6-custom-data-structures",
        [
            "2.6-自定义数据结构.md",
            "2.6.1-Sales_data入门与聚合.md",
            "2.6.2-类内初始值.md",
            "2.6.3-Sales_data完整版.md",
            "2.6.4-友元与前置声明.md",
            "2.6.5-头文件分离与保护.md",
        ],
    ),
    (
        "ch03-strings-vectors-arrays",
        "3.2-library-type-string",
        [
            "3.2-标准库类型string.md",
            "3.2.1-定义与初始化.md",
            "3.2.2-读写操作.md",
            "3.2.3-拼接与内存优化.md",
            "3.2.4-string与C风格字符串.md",
            "3.2.5-cctype字符校验.md",
            "3.2.6-HFT场景拓展.md",
        ],
    ),
    (
        "ch03-strings-vectors-arrays",
        "3.3-library-type-vector",
        [
            "3.3-标准库类型vector.md",
            "3.3.1-动态数组基础.md",
            "3.3.2-圆括号与花括号初始化.md",
            "3.3.3-下标与增删.md",
            "3.3.4-元素类型与嵌套容器.md",
            "3.3.5-拷贝移动与扩容.md",
            "3.3.6-HFT实践与示例.md",
        ],
    ),
    (
        "ch03-strings-vectors-arrays",
        "3.4-introducing-iterators",
        [
            "3.4-迭代器介绍.md",
            "3.4.1-为何需要迭代器.md",
            "3.4.2-begin-end与常量迭代器.md",
            "3.4.3-迭代器运算符与遍历.md",
            "3.4.4-随机访问算术运算.md",
            "3.4.5-易错点失效与考点.md",
        ],
    ),
    (
        "ch03-strings-vectors-arrays",
        "3.5-arrays",
        [
            "3.5-数组.md",
            "3.5.1-定义与初始化.md",
            "3.5.2-下标访问与越界.md",
            "3.5.3-数组退化与指针.md",
            "3.5.4-begin-end与vector对比.md",
            "3.5.5-易错点与示例.md",
        ],
    ),
    (
        "ch04-expressions",
        "4.2-4.10-operators",
        [
            "4.2-4.10各种运算符详解.md",
            "4.2-算术运算符.md",
            "4.3-关系与逻辑运算符.md",
            "4.4-赋值运算符.md",
            "4.5-自增自减.md",
            "4.6-成员访问运算符.md",
            "4.7-条件运算符.md",
            "4.8-位运算符.md",
            "4.9-sizeof运算符.md",
            "4.10-逗号运算符.md",
        ],
    ),
    (
        "ch04-expressions",
        "4.11-type-conversions",
        [
            "4.11-类型转换.md",
            "4.11.1-隐式类型转换.md",
            "4.11.2-显式转换与四种cast.md",
            "4.11.3-规范易错点与示例.md",
        ],
    ),
    (
        "ch04-expressions",
        "4.12-operator-precedence",
        [
            "4.12-运算符优先级表.md",
            "4.12.1-优先级与结合律表.md",
            "4.12.2-求值顺序与括号规范.md",
        ],
    ),
]


def build_reloc_map() -> dict[str, str]:
    """Map old path suffix (forward slashes) -> new path suffix."""
    reloc: dict[str, str] = {}
    for chapter, folder, files in SECTION_FOLDERS:
        for name in files:
            old = f"01-C++Primer/{chapter}/{name}"
            new = f"01-C++Primer/{chapter}/{folder}/{name}"
            reloc[old] = new
            # also chapter-relative for replacement inside same chapter
            reloc[f"{chapter}/{name}"] = f"{chapter}/{folder}/{name}"
    return reloc


def move_files() -> None:
    for chapter, folder, files in SECTION_FOLDERS:
        ch_dir = PRIMER / chapter
        dest_dir = ch_dir / folder
        dest_dir.mkdir(parents=True, exist_ok=True)
        for name in files:
            src = ch_dir / name
            dst = dest_dir / name
            if not src.exists():
                print(f"SKIP missing: {src}")
                continue
            if dst.exists():
                print(f"SKIP exists: {dst}")
                continue
            subprocess.run(
                ["git", "mv", str(src), str(dst)],
                cwd=ROOT,
                check=True,
            )
            print(f"MOVED {src.relative_to(ROOT)} -> {dst.relative_to(ROOT)}")


def build_chapter_file_map() -> dict[tuple[str, str], str]:
    """Map (chapter_dir, filename) -> section folder name."""
    m: dict[tuple[str, str], str] = {}
    for chapter, folder, files in SECTION_FOLDERS:
        for name in files:
            m[(chapter, name)] = folder
    return m


LINK_RE = re.compile(r"\]\((\./)([^)#/]+\.md)\)")


def current_section_folder(rel_posix: str) -> tuple[str, str | None]:
    """Return (chapter_dir, section_folder_or_None)."""
    parts = rel_posix.split("/")
    if len(parts) < 3 or parts[0] != "01-C++Primer":
        return "", None
    chapter = parts[1]
    if len(parts) >= 4:
        for ch, folder, _ in SECTION_FOLDERS:
            if ch == chapter and parts[2] == folder:
                return chapter, folder
    return chapter, None


def fix_relative_section_links(text: str, rel_posix: str, file_map: dict[tuple[str, str], str]) -> str:
    chapter, current_folder = current_section_folder(rel_posix)
    if not chapter:
        return text

    def replacer(match: re.Match[str]) -> str:
        prefix, target = match.group(1), match.group(2)
        target_folder = file_map.get((chapter, target))
        if not target_folder:
            return match.group(0)
        if current_folder is None:
            return f"]({prefix}{target_folder}/{target})"
        if target_folder == current_folder:
            return match.group(0)
        return f"](../{target_folder}/{target})"

    return LINK_RE.sub(replacer, text)


def fix_chapter_root_outbound(text: str, chapter: str) -> str:
    """Chapter-root files linking to moved sections via ./section.md only."""
    for ch, folder, files in SECTION_FOLDERS:
        if ch != chapter:
            continue
        index = files[0]
        text = text.replace(f"](./{index})", f"](./{folder}/{index})")
    return text


def fix_links(reloc: dict[str, str]) -> int:
    file_map = build_chapter_file_map()
    patterns = sorted(reloc.items(), key=lambda x: len(x[0]), reverse=True)
    changed = 0
    for path in ROOT.rglob("*.md"):
        text = path.read_text(encoding="utf-8")
        orig = text
        for old, new in patterns:
            text = text.replace(old, new)

        rel_posix = path.relative_to(ROOT).as_posix()
        text = fix_relative_section_links(text, rel_posix, file_map)
        chapter, current_folder = current_section_folder(rel_posix)
        if chapter and current_folder is None and rel_posix.startswith(f"01-C++Primer/{chapter}/"):
            # single-file at chapter root (not README, not in subfolder)
            if "/" not in rel_posix.removeprefix(f"01-C++Primer/{chapter}/").removesuffix(".md") or rel_posix.count("/") == 3:
                base = rel_posix.removeprefix(f"01-C++Primer/{chapter}/")
                if "/" not in base:
                    text = fix_chapter_root_outbound(text, chapter)

        # Legacy pass: sibling section index links + chapter-root singles
        for chapter_name, folder, files in SECTION_FOLDERS:
            prefix = f"01-C++Primer/{chapter_name}/{folder}/"
            if not rel_posix.startswith(prefix):
                continue
            for other_ch, other_folder, other_files in SECTION_FOLDERS:
                if other_ch != chapter_name:
                    continue
                if other_folder == folder:
                    continue
                index = other_files[0]
                text = text.replace(f"](./{index})", f"](../{other_folder}/{index})")
            ch_dir = PRIMER / chapter_name
            for child in ch_dir.iterdir():
                if child.is_file() and child.suffix == ".md" and child.name != "README.md":
                    name = child.name
                    text = text.replace(f"](./{name})", f"](../{name})")
            break

        if text != orig:
            path.write_text(text, encoding="utf-8")
            changed += 1
            print(f"LINKS {path.relative_to(ROOT)}")
    return changed


def fix_chapter_readmes(reloc: dict[str, str]) -> None:
    """Ensure chapter README uses folder paths for nested items."""
    for chapter, folder, files in SECTION_FOLDERS:
        readme = PRIMER / chapter / "README.md"
        if not readme.exists():
            continue
        text = readme.read_text(encoding="utf-8")
        orig = text
        for name in files:
            old = f"](./{name})"
            new = f"](./{folder}/{name})"
            text = text.replace(old, new)
        if text != orig:
            readme.write_text(text, encoding="utf-8")
            print(f"README {readme.relative_to(ROOT)}")


def main() -> None:
    import sys

    reloc = build_reloc_map()
    if "--links-only" in sys.argv:
        n = fix_links(reloc)
        print(f"Updated {n} markdown files")
        return
    move_files()
    fix_chapter_readmes(reloc)
    n = fix_links(reloc)
    print(f"Updated {n} markdown files")


if __name__ == "__main__":
    main()
