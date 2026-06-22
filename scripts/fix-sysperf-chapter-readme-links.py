#!/usr/bin/env python3
"""Fix cross-module link depth in SysPerf chapter README.md files."""
import re
from pathlib import Path

SYSPERF = Path(__file__).resolve().parents[1] / "02-Systems-Performance-2nd"


def fix_readme(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    new = text
    # chapter README is one level below syperf root
    new = re.sub(r"\(\.\./(0[0-9]|1[01])-", r"(../../\1-", new)
    new = new.replace("..//", "/")
    if new != text:
        path.write_text(new, encoding="utf-8")
        print("fixed", path.relative_to(SYSPERF))


def main() -> None:
    for path in SYSPERF.glob("chapter-*/README.md"):
        fix_readme(path)


if __name__ == "__main__":
    main()
