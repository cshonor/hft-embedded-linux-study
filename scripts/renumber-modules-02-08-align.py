#!/usr/bin/env python3
"""Align folders 02-08 with reading order (2025-06).

  04 Hennessy  -> 02
  02 SysPerf   -> 03
  03 BPF       -> 04
  08 ULK       -> 06
  06 Gorman    -> 07
  07 TLPI      -> 08

Run from repo root: python scripts/renumber-modules-02-08-align.py
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

GIT_MV_TO_TMP = [
    ("04-Computer-Architecture-6th", "_tmp-renum-02-arch"),
    ("02-Systems-Performance-2nd", "_tmp-renum-03-sysperf"),
    ("03-BPF-Performance-Tools", "_tmp-renum-04-bpf"),
    ("08-Understanding-Linux-Kernel", "_tmp-renum-06-ulk"),
    ("06-Linux-Virtual-Memory-Manager", "_tmp-renum-07-gorman"),
    ("07-The-Linux-Programming-Interface", "_tmp-renum-08-tlpi"),
]

GIT_MV_TO_FINAL = [
    ("_tmp-renum-02-arch", "02-Computer-Architecture-6th"),
    ("_tmp-renum-03-sysperf", "13-Systems-Performance-2nd"),
    ("_tmp-renum-04-bpf", "14-BPF-Performance-Tools"),
    ("_tmp-renum-06-ulk", "04-Understanding-Linux-Kernel"),
    ("_tmp-renum-07-gorman", "05-Linux-Virtual-Memory-Manager"),
    ("_tmp-renum-08-tlpi", "06-The-Linux-Programming-Interface"),
]

# old path -> placeholder -> new path (longest first in pass 1)
PATH_SWAP = [
    ("08-Understanding-Linux-Kernel", "@@MOD06-ULK@@"),
    ("07-The-Linux-Programming-Interface", "@@MOD08-TLPI@@"),
    ("06-Linux-Virtual-Memory-Manager", "@@MOD07-GORMAN@@"),
    ("04-Computer-Architecture-6th", "@@MOD02-ARCH@@"),
    ("03-BPF-Performance-Tools", "@@MOD04-BPF@@"),
    ("02-Systems-Performance-2nd", "@@MOD03-SYSPERF@@"),
]

PLACEHOLDER_TO_NEW = [
    ("@@MOD06-ULK@@", "04-Understanding-Linux-Kernel"),
    ("@@MOD08-TLPI@@", "06-The-Linux-Programming-Interface"),
    ("@@MOD07-GORMAN@@", "05-Linux-Virtual-Memory-Manager"),
    ("@@MOD02-ARCH@@", "02-Computer-Architecture-6th"),
    ("@@MOD04-BPF@@", "14-BPF-Performance-Tools"),
    ("@@MOD03-SYSPERF@@", "13-Systems-Performance-2nd"),
]

# Shorthand / prose (after path tokens updated)
PROSE_REPLACEMENTS = [
    ("04-Hennessy", "02-Hennessy"),
    ("04 Hennessy", "02 Hennessy"),
    ("`04` Hennessy", "`02` Hennessy"),
    ("| **12** |", "| **02** |"),
    ("01(+04)", "01 → 02"),
    ("01 + **04**", "**01** → **02**"),
    ("**01** + **04**", "**01** → **02**"),
    ("CSAPP (+ **04** Hennessy", "CSAPP → **02** Hennessy"),
    ("02 SysPerf", "03 SysPerf"),
    ("02-SysPerf", "03-SysPerf"),
    ("| **02** | SysPerf", "| **11** | SysPerf"),
    ("`02` SysPerf", "`03` SysPerf"),
    ("03 BPF", "04 BPF"),
    ("03-BPF", "04-BPF"),
    ("| **11** | BPF", "| **12** | BPF"),
    ("08 ULK", "06 ULK"),
    ("08-ULK", "06-ULK"),
    ("| **06** | ULK", "| **04** | ULK"),
    ("`08` ULK", "`06` ULK"),
    ("`08 ULK`", "`06 ULK`"),
    ("06 Gorman", "07 Gorman"),
    ("06-Gorman", "07-Gorman"),
    ("| **04** | Gorman", "| **05** | Gorman"),
    ("07 TLPI", "08 TLPI"),
    ("07-TLPI", "08-TLPI"),
    ("| **05** | TLPI", "| **06** | TLPI"),
    ("`07` TLPI", "`08` TLPI"),
    ("05 → 08 ULK", "05 → 06 ULK"),
    ("05 → 08 → 06", "05 → 06 → 07"),
    ("05 → 08 ULK → 06 Gorman → 07 TLPI", "05 → 06 ULK → 05 Gorman → 06 TLPI"),
    ("LKD → 08 ULK", "LKD → 06 ULK"),
    ("LKD → 08 ULK（", "LKD → 06 ULK（"),
    ("05 LKD → 08 ULK", "05 LKD → 06 ULK"),
    ("05 LKD → 08 ULK → 06 Gorman", "05 LKD → 06 ULK → 07 Gorman"),
    ("05 LKD → 08 ULK → 06 Gorman → 07 TLPI", "03 LKD → 04 ULK → 05 Gorman → 06 TLPI"),
    ("`05` LKD → `08` ULK", "`05` LKD → `06` ULK"),
    ("`05` LKD → `08` ULK → `06` Gorman → `07` TLPI", "`03` LKD → `04` ULK → `05` Gorman → `06` TLPI"),
    ("00 → 01 → 04 → 02 → 03 → 05 → 08 → 06 → 07", "00 → 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08"),
    ("01 → 04 → 02 → 03", "01 → 02 → 03 → 04"),
    ("01(+04) → 02 → 03 → 05 → 08 → 06 → 07", "01 → 02 → 03 → 04 → 05 → 06 → 07 → 08"),
    ("01 → 04 → 02 → 03 → 05 → 08 → 06 → 07", "01 → 02 → 03 → 04 → 05 → 06 → 07 → 08"),
    ("01  CSAPP + 04 Hennessy", "01  CSAPP → 02  Hennessy"),
    ("04  Hennessy              ← 原 02 位", "02  Hennessy"),
    ("02  SysPerf → 03  BPF      ← 原 02–03 顺延", "03  SysPerf → 04  BPF"),
    ("05  LKD → 08 ULK           ← 内核概念", "05  LKD → 06 ULK"),
    ("06  Gorman → 07  TLPI", "07  Gorman → 08  TLPI"),
    ("01(+04)", "01 → 02"),
    ("紧接 01，再进性能篇", "紧接 01 CSAPP"),
    ("读序 2–3", "读序 2–3"),  # no-op keep
    ("读序 4–5", "读序 4–5"),
    ("读序 6–7", "读序 6–7"),
    ("读序 6–8", "读序 6–8"),
    ("`03`–`06`", "`03`–`06`"),
    ("03–06         07–12", "03–06         07–12"),
    ("内核段：** `05` LKD → `08` ULK → `06` Gorman → `07` TLPI", "内核段：** `03` LKD → `04` ULK → `05` Gorman → `06` TLPI"),
    ("**内核段：** `05` LKD → `08` ULK → `06` Gorman → `07` TLPI", "**内核段：** `03` LKD → `04` ULK → `05` Gorman → `06` TLPI"),
    ("→ [04-Hennessy", "→ [02-Hennessy"),
    ("[04-Hennessy", "[02-Hennessy"),
    ("[02-SysPerf", "[13-Systems-Performance"),
    ("[03-BPF", "[14-BPF"),
    ("[08-ULK", "[04-Understanding-Linux-Kernel"),
    ("[06-Gorman", "[05-Linux-Virtual-Memory-Manager"),
    ("[07-TLPI", "[06-The-Linux-Programming-Interface"),
    ("02 SysPerf Ch", "03 SysPerf Ch"),
    ("02 SysPerf §", "03 SysPerf §"),
    ("04 Hennessy Ch", "02 Hennessy Ch"),
    ("08 ULK Ch", "06 ULK Ch"),
    ("08-ULK Ch", "06-ULK Ch"),
    ("与 [02-SysPerf", "与 [13-Systems-Performance"),
    ("与 [04-Hennessy", "与 [02-Hennessy"),
    ("→ [02-SysPerf", "→ [13-Systems-Performance"),
    ("→ [04-Hennessy", "→ [02-Hennessy"),
    ("→ [08-ULK", "→ [04-Understanding-Linux-Kernel"),
    ("→ [07-TLPI", "→ [06-The-Linux-Programming-Interface"),
    ("→ [06-Gorman", "→ [05-Linux-Virtual-Memory-Manager"),
    ("../02-Systems-Performance", "../13-Systems-Performance"),
    ("../03-BPF", "../14-BPF"),
    ("../04-Computer-Architecture", "../02-Computer-Architecture"),
    ("../06-Linux-Virtual", "../05-Linux-Virtual"),
    ("../07-The-Linux", "../06-The-Linux"),
    ("../08-Understanding", "../04-Understanding"),
    ("../../02-Systems-Performance", "../../13-Systems-Performance"),
    ("../../03-BPF", "../../14-BPF"),
    ("../../04-Computer-Architecture", "../../02-Computer-Architecture"),
    ("../../06-Linux-Virtual", "../../05-Linux-Virtual"),
    ("../../07-The-Linux", "../../06-The-Linux"),
    ("../../08-Understanding", "../../04-Understanding"),
    ("../../../02-Systems-Performance", "../../../13-Systems-Performance"),
    ("../../../03-BPF", "../../../14-BPF"),
    ("../../../04-Computer-Architecture", "../../../02-Computer-Architecture"),
    ("../../../06-Linux-Virtual", "../../../05-Linux-Virtual"),
    ("../../../07-The-Linux", "../../../06-The-Linux"),
    ("../../../08-Understanding", "../../../04-Understanding"),
    ("../../../../02-Systems-Performance", "../../../../13-Systems-Performance"),
    ("../../../../03-BPF", "../../../../14-BPF"),
    ("../../../../04-Computer-Architecture", "../../../../02-Computer-Architecture"),
    ("../../../../06-Linux-Virtual", "../../../../05-Linux-Virtual"),
    ("../../../../07-The-Linux", "../../../../06-The-Linux"),
    ("../../../../08-Understanding", "../../../../04-Understanding"),
    ("[12-HFT](../15-HFT", "[15-HFT](../15-HFT"),  # fix wrong refs if any
    ("下一本：** [02-SysPerf", "下一本：** [13-Systems-Performance"),
    ("下一本：** [04-Computer", "下一本：** [02-Computer"),
    ("Hennessy 理论 → [04-Computer", "Hennessy 理论 → [02-Computer"),
    ("紧接 [02-SysPerf", "紧接 [13-Systems-Performance"),
    ("紧接 [02-SysPerf", "紧接 [13-Systems-Performance"),
    ("Gregg 双书 · 02 → 03", "Gregg 双书 · 13 → 14"),
    ("| **02** | SysPerf | **11** | BPF |", "| **11** | SysPerf | **12** | BPF |"),
    ("| **03** | LKD → **08** ULK |", "| **03** | LKD → **06** ULK |"),
    ("| **06** → **07** | Gorman → TLPI |", "| **07** → **08** | Gorman → TLPI |"),
    ("读序 · 非目录编号", "文件夹编号 = 读序"),
    ("文件夹名 `00`–`16` 不变**；**读序**", "文件夹 `00`–`16`"),
    ("| 读序 | 文件夹 |", "| 序号 | 文件夹 |"),
]

SKIP_DIRS = {".git", "node_modules", ".cursor", "__pycache__"}
SKIP_FILES = {
    "renumber-modules-02-08-align.py",
    "renumber-modules-08-ulk.py",
    "renumber-modules-07-tlpi.py",
}


def git_mv(src: str, dst: str) -> None:
    r = subprocess.run(["git", "mv", src, dst], cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"git mv failed: {src} -> {dst}\n{r.stderr}", file=sys.stderr)
        sys.exit(1)
    print(f"git mv {src} -> {dst}")


def apply_replacements(text: str) -> str:
    for old, new in PATH_SWAP:
        text = text.replace(old, new)
    for old, new in PLACEHOLDER_TO_NEW:
        text = text.replace(old, new)
    for old, new in PROSE_REPLACEMENTS:
        text = text.replace(old, new)
    return text


def update_text_files() -> int:
    count = 0
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if any(p in SKIP_DIRS for p in path.parts):
            continue
        if path.name in SKIP_FILES:
            continue
        if path.suffix.lower() not in {".md", ".py", ".mdc", ".txt", ".yml", ".yaml", ".json"}:
            continue
        try:
            original = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        updated = apply_replacements(original)
        if updated != original:
            path.write_text(updated, encoding="utf-8", newline="\n")
            count += 1
            print(f"updated {path.relative_to(ROOT)}")
    return count


def main() -> None:
    for src, dst in GIT_MV_TO_TMP:
        if (ROOT / src).exists():
            git_mv(src, dst)
        else:
            print(f"skip missing: {src}")

    for src, dst in GIT_MV_TO_FINAL:
        if (ROOT / src).exists():
            git_mv(src, dst)
        else:
            print(f"skip missing: {src}")

    n = update_text_files()
    print(f"\nDone. Updated {n} text files.")


if __name__ == "__main__":
    main()
