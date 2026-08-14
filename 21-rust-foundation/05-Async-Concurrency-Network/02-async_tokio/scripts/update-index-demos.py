# -*- coding: utf-8 -*-
"""Set Demo column in each 本章学习笔记.md when X.Y-slug/ exists."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

ROW_RE = re.compile(
    r"^(\| \d+\.\d+ \|.*?\| \[[^\]]+\.md\]\([^)]+\) \|) ([^|]*) (\|)\s*$"
)
SLUG_RE = re.compile(r"\[([^\]]+\.md)\]")


def main() -> None:
    for ch_dir in sorted(ROOT.glob("ch*")):
        idx = ch_dir / "本章学习笔记.md"
        if not idx.exists():
            continue
        lines = idx.read_text(encoding="utf-8").splitlines()
        out, changed = [], False
        for line in lines:
            m = ROW_RE.match(line)
            if not m:
                out.append(line)
                continue
            sm = SLUG_RE.search(line)
            slug = sm.group(1).replace(".md", "") if sm else ""
            link = f"[{slug}/](./{slug}/)" if slug and (ch_dir / slug).is_dir() else "—"
            new_line = f"{m.group(1)} {link} {m.group(3)}"
            if new_line != line:
                changed = True
            out.append(new_line)
        if changed:
            idx.write_text("\n".join(out) + "\n", encoding="utf-8")
            print("updated:", idx.relative_to(ROOT))
    print("Done.")


if __name__ == "__main__":
    main()
