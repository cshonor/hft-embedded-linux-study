#!/usr/bin/env python3
"""Patch ER Item markdown: demo status + §6 link to ER-拓展索引."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CHAPTERS = list(ROOT.glob("Chapter-*"))

# item_num -> (demo_done, demo_line)
DEMO = {
    3: (True, "- [x] [item-03-option-result](../ER-demos/item-03-option-result/)"),
    4: (True, "- [x] [item-04-error-types](../ER-demos/item-04-error-types/) · [item-04-core-error](../ER-demos/item-04-core-error/)"),
    5: (True, "- [x] [item-05-06-newtype](../ER-demos/item-05-06-newtype/)（Deref / AsRef·Into / 过度 Deref）"),
    6: (True, "- [x] [item-05-06-newtype](../ER-demos/item-05-06-newtype/)（derive_more / over_deref）"),
    16: (True, "- [x] [item-16-miri](../ER-demos/item-16-miri/) + CI miri job"),
    15: (True, "- [x] [item-15-borrow-checker](../ER-demos/item-15-borrow-checker/)"),
    18: (True, "- [x] [item-18-dont-panic](../ER-demos/item-18-dont-panic/)"),
    20: (True, "- [x] [item-20-tlv](../ER-demos/item-20-tlv/)"),
    21: (True, "- [x] [WORKSPACE.md MSRV](../ER-demos/WORKSPACE.md)"),
    22: (True, "- [x] [item-22-visibility](../ER-demos/item-22-visibility/)"),
    24: (True, "- [x] [item-24-re-export](../ER-demos/item-24-re-export/)"),
    25: (True, "- [x] [WORKSPACE.md](../ER-demos/WORKSPACE.md)"),
    26: (True, "- [x] [item-26-feature-creep](../ER-demos/item-26-feature-creep/)"),
    29: (True, "- [x] [clippy.toml](../ER-demos/clippy.toml)"),
    30: (True, "- [x] [item-30-black-box](../ER-demos/item-30-black-box/)"),
    32: (True, "- [x] [CI 示例](../../.github/workflows/er-study-ci.yml)"),
    33: (True, "- [x] [item-33-no-std](../ER-demos/item-33-no-std/)"),
    34: (True, "- [x] [item-34-ffi-box](../ER-demos/item-34-ffi-box/)"),
    35: (True, "- [x] [item-35-bindgen](../ER-demos/item-35-bindgen/) · [item-35-sys-workspace](../ER-demos/item-35-sys-workspace/)"),
}

SECTION6 = """## 6. 后续拓展

> 展开版：[ER-拓展索引 § Item {num:02d}](../ER-拓展索引.md#item-{num:02d})

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。"""


def item_num_from_path(p: Path) -> int | None:
    m = re.search(r"Item-(\d+)-", p.name)
    return int(m.group(1)) if m else None


def patch_file(path: Path) -> bool:
    n = item_num_from_path(path)
    if n is None:
        return False
    text = path.read_text(encoding="utf-8")
    orig = text

    # demo line in ## 状态
    demo_info = DEMO.get(n)
    if demo_info:
        done, line = demo_info
        text = re.sub(
            r"- \[ \] demo[^\n]*",
            line,
            text,
            count=1,
        )
    else:
        text = re.sub(
            r"- \[ \] demo / 代码练习",
            "- [ ] demo（见 [ER-demos 索引](../ER-demos/README.md) / Book demo）",
            text,
            count=1,
        )

    # replace ## 6. ... through --- before ## 记忆卡片 or EOF section
    pattern = r"## 6\. 后续拓展[^\n]*\n\n(?:.*?\n\n)?(?=---\n\n## 记忆卡片)"
    replacement = SECTION6.format(num=n) + "\n\n"
    text, count = re.subn(pattern, replacement, text, flags=re.DOTALL)
    if count == 0:
        # Item 32 slightly different header
        pattern2 = r"## 6\. 后续拓展\n\n.*?(?=\n---\n\n## 记忆卡片)"
        text, count = re.subn(pattern2, SECTION6.format(num=n), text, flags=re.DOTALL)

    if text != orig:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> None:
    changed = 0
    for ch in CHAPTERS:
        for p in sorted(ch.glob("Item-*.md")):
            if patch_file(p):
                changed += 1
                print("patched", p.relative_to(ROOT))
    print(f"done: {changed} files")


if __name__ == "__main__":
    main()
