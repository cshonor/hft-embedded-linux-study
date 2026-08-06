#!/usr/bin/env python3
"""Rename Primer section folders from Chinese to English and fix links."""
from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRIMER = ROOT / "01-C++Primer"

# old_folder_name -> new_folder_name (within same chapter)
FOLDER_RENAMES: dict[str, str] = {
    "1.5-a-brief-introduction-to-classes": "1.5-a-brief-introduction-to-classes",
    "2.2-variables": "2.2-variables",
    "2.3-compound-types": "2.3-compound-types",
    "2.4-const-qualifier": "2.4-const-qualifier",
    "2.6-custom-data-structures": "2.6-custom-data-structures",
    "3.2-library-type-string": "3.2-library-type-string",
    "3.3-library-type-vector": "3.3-library-type-vector",
    "3.4-introducing-iterators": "3.4-introducing-iterators",
    "3.5-arrays": "3.5-arrays",
    "4.2-4.10-operators": "4.2-4.10-operators",
    "4.11-type-conversions": "4.11-type-conversions",
    "4.12-operator-precedence": "4.12-operator-precedence",
}


def rename_folders() -> None:
    for chapter_dir in PRIMER.iterdir():
        if not chapter_dir.is_dir():
            continue
        for old_name, new_name in FOLDER_RENAMES.items():
            src = chapter_dir / old_name
            dst = chapter_dir / new_name
            if not src.is_dir():
                continue
            if dst.exists():
                print(f"SKIP exists: {dst}")
                continue
            subprocess.run(
                ["git", "mv", str(src), str(dst)],
                cwd=ROOT,
                check=True,
            )
            print(f"RENAMED {src.relative_to(ROOT)} -> {dst.relative_to(ROOT)}")


# Wrong link targets (from overly broad replace) -> actual Chinese filenames on disk
FILENAME_LINK_FIXES: dict[str, str] = {
    "1.5-a-brief-introduction-to-classes.md": "1.5-类简介.md",
    "2.2-variables.md": "2.2-变量.md",
    "2.2.2-variables常量与static.md": "2.2.2-变量常量与static.md",
    "2.3-compound-types.md": "2.3-复合类型.md",
    "2.4-const-qualifier.md": "2.4-const限定符.md",
    "2.6-custom-data-structures.md": "2.6-自定义数据结构.md",
    "3.2-library-type-string.md": "3.2-标准库类型string.md",
    "3.3-library-type-vector.md": "3.3-标准库类型vector.md",
    "3.4-introducing-iterators.md": "3.4-迭代器介绍.md",
    "3.5-arrays.md": "3.5-数组.md",
    "4.2-4.10-operators.md": "4.2-4.10各种运算符详解.md",
    "4.11-type-conversions.md": "4.11-类型转换.md",
    "4.12-operator-precedence.md": "4.12-运算符优先级表.md",
}


def fix_links() -> int:
    patterns = sorted(FOLDER_RENAMES.items(), key=lambda x: len(x[0]), reverse=True)
    changed = 0
    for path in ROOT.rglob("*"):
        if path.suffix not in {".md", ".py"}:
            continue
        if path.name == "rename_section_folders_en.py":
            continue
        text = path.read_text(encoding="utf-8")
        orig = text
        for old, new in patterns:
            text = text.replace(f"/{old}/", f"/{new}/")
            text = text.replace(f"\\{old}\\", f"\\{new}\\")
        for wrong, right in FILENAME_LINK_FIXES.items():
            text = text.replace(wrong, right)
        if text != orig:
            path.write_text(text, encoding="utf-8")
            changed += 1
            print(f"LINKS {path.relative_to(ROOT)}")
    return changed


def main() -> None:
    rename_folders()
    n = fix_links()
    print(f"Updated {n} files")


if __name__ == "__main__":
    main()
