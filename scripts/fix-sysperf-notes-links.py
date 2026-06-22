#!/usr/bin/env python3
"""Fix relative links in SysPerf notes/ (depth = chapter/notes/)."""
import re
from pathlib import Path

SYSPERF = Path(__file__).resolve().parents[1] / "02-Systems-Performance-2nd"

# Repo-root modules (not under 02-Systems-Performance-2nd/)
REPO_PREFIXES = (
    "00-", "01-", "03-", "04-", "05-", "06-", "07-", "08-", "09-", "10-DPDK", "11-",
)

BROKEN_NOTE = re.compile(
    r"\(\.\./(?:\.\./)?chapter-(\d{2})-[^/]+/notes/[^)]+\)"
)


def fix_notes_file(text: str) -> str:
    # Intra-SysPerf sibling chapters / appendix / OUTLINE from notes/
    text = text.replace("](../chapter-", "](../../chapter-")
    text = text.replace("](../appendix-", "](../../appendix-")
    text = text.replace("](../OUTLINE", "](../../OUTLINE")
    text = text.replace("](../CROSS-MODULE", "](../../CROSS-MODULE")
    text = text.replace("..//", "/")

    # Cross-repo links need ../../../ from notes/
    for prefix in REPO_PREFIXES:
        text = text.replace(f"](../../{prefix}", f"](../../../{prefix}")
        text = text.replace(f"](../{prefix}", f"](../../../{prefix}")

    def repl(m: re.Match) -> str:
        num = m.group(1)
        for d in SYSPERF.glob(f"chapter-{num}-*"):
            return f"(../../{d.name}/)"
        return m.group(0)

    return BROKEN_NOTE.sub(repl, text)


def main():
    for path in SYSPERF.rglob("notes/*.md"):
        old = path.read_text(encoding="utf-8")
        new = fix_notes_file(old)
        if new != old:
            path.write_text(new, encoding="utf-8")
            print("fixed", path.relative_to(SYSPERF))


if __name__ == "__main__":
    main()
