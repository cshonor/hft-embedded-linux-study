#!/usr/bin/env python3
"""Post-fix hub READMEs after split."""
import re
from pathlib import Path

ER_ROOT = Path(__file__).resolve().parent.parent

for readme in ER_ROOT.glob("Chapter-*/Item-*/README.md"):
    text = readme.read_text(encoding="utf-8")
    orig = text
    text = re.sub(r"---\n\n---\n\n## 一句话", "---\n\n## 一句话", text)
    if "> 展开版：[ER-拓展" in text and "## 后续拓展" not in text:
        text = text.replace("\n> 展开版：[ER-拓展", "\n## 后续拓展\n\n> 展开版：[ER-拓展")
    if text != orig:
        readme.write_text(text, encoding="utf-8")
        print(f"fixed {readme.parent.name}")

print("Done.")
