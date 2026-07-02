#!/usr/bin/env python3
"""Renumber modules 03-14: shift 05-14 up to 03-12; SysPerf/BPF to 13/14.

  03 SysPerf   -> 13
  04 BPF       -> 14
  05 LKD       -> 03
  06 ULK       -> 04
  07 Gorman    -> 05
  08 TLPI      -> 06
  09 OS        -> 07
  10 PNP       -> 08
  11 UNP       -> 09
  12 TCP/IP    -> 10
  13 Rosen     -> 11
  14 DPDK      -> 12

Run: py scripts/renumber-modules-03-14-perf-defer.py
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

GIT_MV_TO_TMP = [
    ("03-Systems-Performance-2nd", "_tmp-rn-13-sysperf"),
    ("04-BPF-Performance-Tools", "_tmp-rn-14-bpf"),
    ("05-Linux-Kernel-Development", "_tmp-rn-03-lkd"),
    ("06-Understanding-Linux-Kernel", "_tmp-rn-04-ulk"),
    ("07-Linux-Virtual-Memory-Manager", "_tmp-rn-05-gorman"),
    ("08-The-Linux-Programming-Interface", "_tmp-rn-06-tlpi"),
    ("09-system-low-level-hands-on", "_tmp-rn-07-os"),
    ("10-Practical-Network-Programming", "_tmp-rn-08-pnp"),
    ("11-UNP-Vol1", "_tmp-rn-09-unp"),
    ("12-TCP-IP-Illustrated-Vol1", "_tmp-rn-10-tcpip"),
    ("13-Linux-Kernel-Networking", "_tmp-rn-11-rosen"),
    ("14-DPDK-Low-Latency-Network", "_tmp-rn-12-dpdk"),
]

GIT_MV_TO_FINAL = [
    ("_tmp-rn-03-lkd", "03-Linux-Kernel-Development"),
    ("_tmp-rn-04-ulk", "04-Understanding-Linux-Kernel"),
    ("_tmp-rn-05-gorman", "05-Linux-Virtual-Memory-Manager"),
    ("_tmp-rn-06-tlpi", "06-The-Linux-Programming-Interface"),
    ("_tmp-rn-07-os", "07-system-low-level-hands-on"),
    ("_tmp-rn-08-pnp", "08-Practical-Network-Programming"),
    ("_tmp-rn-09-unp", "09-UNP-Vol1"),
    ("_tmp-rn-10-tcpip", "10-TCP-IP-Illustrated-Vol1"),
    ("_tmp-rn-11-rosen", "11-Linux-Kernel-Networking"),
    ("_tmp-rn-12-dpdk", "12-DPDK-Low-Latency-Network"),
    ("_tmp-rn-13-sysperf", "13-Systems-Performance-2nd"),
    ("_tmp-rn-14-bpf", "14-BPF-Performance-Tools"),
]

# high -> low to avoid prefix collisions
PATH_SWAP = [
    ("14-DPDK-Low-Latency-Network", "@@M12-DPDK@@"),
    ("13-Linux-Kernel-Networking", "@@M11-ROSEN@@"),
    ("12-TCP-IP-Illustrated-Vol1", "@@M10-TCPIP@@"),
    ("11-UNP-Vol1", "@@M09-UNP@@"),
    ("10-Practical-Network-Programming", "@@M08-PNP@@"),
    ("09-system-low-level-hands-on", "@@M07-OS@@"),
    ("08-The-Linux-Programming-Interface", "@@M06-TLPI@@"),
    ("07-Linux-Virtual-Memory-Manager", "@@M05-GORMAN@@"),
    ("06-Understanding-Linux-Kernel", "@@M04-ULK@@"),
    ("05-Linux-Kernel-Development", "@@M03-LKD@@"),
    ("04-BPF-Performance-Tools", "@@M14-BPF@@"),
    ("03-Systems-Performance-2nd", "@@M13-SYSPERF@@"),
]

PLACEHOLDER_TO_NEW = [
    ("@@M12-DPDK@@", "12-DPDK-Low-Latency-Network"),
    ("@@M11-ROSEN@@", "11-Linux-Kernel-Networking"),
    ("@@M10-TCPIP@@", "10-TCP-IP-Illustrated-Vol1"),
    ("@@M09-UNP@@", "09-UNP-Vol1"),
    ("@@M08-PNP@@", "08-Practical-Network-Programming"),
    ("@@M07-OS@@", "07-system-low-level-hands-on"),
    ("@@M06-TLPI@@", "06-The-Linux-Programming-Interface"),
    ("@@M05-GORMAN@@", "05-Linux-Virtual-Memory-Manager"),
    ("@@M04-ULK@@", "04-Understanding-Linux-Kernel"),
    ("@@M03-LKD@@", "03-Linux-Kernel-Development"),
    ("@@M14-BPF@@", "14-BPF-Performance-Tools"),
    ("@@M13-SYSPERF@@", "13-Systems-Performance-2nd"),
]

PROSE_REPLACEMENTS = [
    # execution chains (longest first)
    ("00 → 01 → 02 → 05 → 06 → 07 → 08 → 09/01 → 17 → 10 → 11 → 01网络 → 12 → 13 → 14 → 03 → 04 → 15 → 16",
     "00 → 01 → 02 → 03 → 04 → 05 → 06 → 07/01 → 17 → 08 → 09 → 01网络 → 10 → 11 → 12 → 13 → 14 → 15 → 16"),
    ("05 LKD → 06 ULK → 07 Gorman → 08 TLPI", "03 LKD → 04 ULK → 05 Gorman → 06 TLPI"),
    ("`05`–`08`", "`03`–`06`"),
    ("05–08         09–14", "03–06         07–12"),
    ("05–08       09         10–14    03–04      15–16", "03–06       07         08–12    13–14      15–16"),
    ("05–08       07         08–12", "03–06       07         08–12"),
    ("10–14 网络", "08–12 网络"),
    ("10–14 网络栈", "08–12 网络栈"),
    ("10–14 网络/DPDK", "08–12 网络/DPDK"),
    ("03 SysPerf → 04  BPF", "13 SysPerf → 14 BPF"),
    ("03 SysPerf → 04 BPF", "13 SysPerf → 14 BPF"),
    ("`03`/`04`", "`13`/`14`"),
    ("`03` SysPerf、`04` BPF", "`13` SysPerf、`14` BPF"),
    ("folders 03/04", "folders 13/14"),
    ("文件夹 03/04", "文件夹 13/14"),
    ("09/01-mikan-os", "07/01-mikan-os"),
    ("09/02-30days-os", "07/02-30days-os"),
    ("09/01", "07/01"),
    ("09/02", "07/02"),
    ("`09/01`", "`07/01`"),
    ("`09/02`", "`07/02`"),
    ("09 自制 OS", "07 自制 OS"),
    ("`09` 本模块", "`07` 本模块"),
    ("`09` 自制", "`07` 自制"),
    ("| **09** |", "| **07** |"),
    ("| **09/01** |", "| **07/01** |"),
    ("| **09/02** |", "| **07/02** |"),
    ("| **10–14** |", "| **08–12** |"),
    ("| **03** |", "| **13** |"),
    ("| **04** |", "| **14** |"),
    ("| **05** |", "| **03** |"),
    ("| **06** |", "| **04** |"),
    ("| **07** |", "| **05** |"),
    ("| **08** |", "| **06** |"),
    ("| **10** |", "| **08** |"),
    ("| **11** |", "| **09** |"),
    ("| **12** |", "| **10** |"),
    ("| **13** |", "| **11** |"),
    ("| **14** |", "| **12** |"),
    ("**文件夹 03**", "**文件夹 13**"),
    ("**文件夹 04**", "**文件夹 14**"),
    ("**文件夹 05**", "**文件夹 03**"),
    ("文件夹 `03`", "文件夹 `13`"),
    ("Gregg 双书 · 03 → 04", "Gregg 双书 · 13 → 14"),
    ("03 读完立刻 04", "13 读完立刻 14"),
    ("紧接 03", "紧接 13"),
    ("紧接 [03-SysPerf", "紧接 [13-Systems-Performance"),
    ("[03-SysPerf", "[13-Systems-Performance"),
    ("[04-BPF", "[14-BPF"),
    ("[05-LKD", "[03-Linux-Kernel-Development"),
    ("[06-ULK", "[04-Understanding-Linux-Kernel"),
    ("[07-Gorman", "[05-Linux-Virtual-Memory-Manager"),
    ("[08-TLPI", "[06-The-Linux-Programming-Interface"),
    ("[09-system", "[07-system"),
    ("[10-Practical", "[08-Practical"),
    ("[11-UNP", "[09-UNP"),
    ("[12-TCP", "[10-TCP"),
    ("[13-Linux-Kernel-Networking", "[11-Linux-Kernel-Networking"),
    ("[14-DPDK", "[12-DPDK"),
    ("../03-Systems-Performance", "../13-Systems-Performance"),
    ("../04-BPF", "../14-BPF"),
    ("../05-Linux-Kernel", "../03-Linux-Kernel"),
    ("../06-Understanding", "../04-Understanding"),
    ("../07-Linux-Virtual", "../05-Linux-Virtual"),
    ("../08-The-Linux", "../06-The-Linux"),
    ("../09-system", "../07-system"),
    ("../10-Practical", "../08-Practical"),
    ("../11-UNP", "../09-UNP"),
    ("../12-TCP", "../10-TCP"),
    ("../13-Linux-Kernel-Networking", "../11-Linux-Kernel-Networking"),
    ("../14-DPDK", "../12-DPDK"),
    ("../../03-Systems-Performance", "../../13-Systems-Performance"),
    ("../../04-BPF", "../../14-BPF"),
    ("../../05-Linux-Kernel", "../../03-Linux-Kernel"),
    ("../../06-Understanding", "../../04-Understanding"),
    ("../../07-Linux-Virtual", "../../05-Linux-Virtual"),
    ("../../08-The-Linux", "../../06-The-Linux"),
    ("../../09-system", "../../07-system"),
    ("../../10-Practical", "../../08-Practical"),
    ("../../11-UNP", "../../09-UNP"),
    ("../../12-TCP", "../../10-TCP"),
    ("../../13-Linux-Kernel-Networking", "../../11-Linux-Kernel-Networking"),
    ("../../14-DPDK", "../../12-DPDK"),
    ("../../../03-Systems-Performance", "../../../13-Systems-Performance"),
    ("../../../04-BPF", "../../../14-BPF"),
    ("../../../05-Linux-Kernel", "../../../03-Linux-Kernel"),
    ("../../../06-Understanding", "../../../04-Understanding"),
    ("../../../07-Linux-Virtual", "../../../05-Linux-Virtual"),
    ("../../../08-The-Linux", "../../../06-The-Linux"),
    ("../../../09-system", "../../../07-system"),
    ("../../../10-Practical", "../../../08-Practical"),
    ("../../../11-UNP", "../../../09-UNP"),
    ("../../../12-TCP", "../../../10-TCP"),
    ("../../../13-Linux-Kernel-Networking", "../../../11-Linux-Kernel-Networking"),
    ("../../../14-DPDK", "../../../12-DPDK"),
    ("../../../../03-Systems-Performance", "../../../../13-Systems-Performance"),
    ("../../../../04-BPF", "../../../../14-BPF"),
    ("../../../../05-Linux-Kernel", "../../../../03-Linux-Kernel"),
    ("../../../../06-Understanding", "../../../../04-Understanding"),
    ("../../../../07-Linux-Virtual", "../../../../05-Linux-Virtual"),
    ("../../../../08-The-Linux", "../../../../06-The-Linux"),
    ("../../../../09-system", "../../../../07-system"),
    ("../../../../10-Practical", "../../../../08-Practical"),
    ("../../../../11-UNP", "../../../../09-UNP"),
    ("../../../../12-TCP", "../../../../10-TCP"),
    ("../../../../13-Linux-Kernel-Networking", "../../../../11-Linux-Kernel-Networking"),
    ("../../../../14-DPDK", "../../../../12-DPDK"),
    ("`05` LKD → `06` ULK → `07` Gorman → `08` TLPI", "`03` LKD → `04` ULK → `05` Gorman → `06` TLPI"),
    ("`14` DPDK → `03`–`04`", "`12` DPDK → `13`–`14`"),
    ("09 之后、10 前", "07 之后、08 前"),
    ("09 之后、10 PNP 之前", "07 之后、08 PNP 之前"),
    ("`09` 之后、`10` PNP", "`07` 之后、`08` PNP"),
    ("15-HFT", "15-HFT"),  # no-op
    ("16-Rust", "16-Rust"),
    ("17-cpp", "17-cpp"),
    ("07 Gorman → 08 TLPI", "05 Gorman → 06 TLPI"),
    ("07 Gorman → 08 TLPI → 09", "05 Gorman → 06 TLPI → 07"),
    ("12 TCP/IP → 13  Rosen → 14  DPDK", "10 TCP/IP → 11 Rosen → 12 DPDK"),
    ("12 TCP/IP → 13 Rosen → 14 DPDK", "10 TCP/IP → 11 Rosen → 12 DPDK"),
    ("→ 10 PNP", "→ 08 PNP"),
    ("→ `10` PNP", "→ `08` PNP"),
    ("10 PNP → 11 UNP → 12–14", "08 PNP → 09 UNP → 10–12"),
    ("10 PNP → 11 UNP → 12–14 网络", "08 PNP → 09 UNP → 10–12 网络"),
    ("前置：** `06` Gorman + **`08` TLPI**", "前置：** `05` Gorman + **`06` TLPI**"),
    ("`06` Gorman + **`08` TLPI**", "`05` Gorman + **`06` TLPI**"),
    ("建议 05–08 后", "建议 03–06 后"),
    ("05–08 后", "03–06 后"),
    ("08 TLPI + 09/01 MikanOS + 10–14", "06 TLPI + 07/01 MikanOS + 08–12"),
    ("08 TLPI + 09/01", "06 TLPI + 07/01"),
    ("14 之后或 15 之前", "12 DPDK 之后、15 HFT 之前"),
    ("14 DPDK 之后", "12 DPDK 之后"),
    ("14 之后", "12 之后"),
    ("HFT 最短路径（当前）：** `01` CSAPP → `02` Hennessy → `05`–`08` 内核/TLPI → `09/01` MikanOS → `14` DPDK → `03`–`04` 观测 → `15` HFT",
     "HFT 最短路径：** `01` CSAPP → `02` Hennessy → `03`–`06` 内核/TLPI → `07/01` MikanOS → `12` DPDK → `13`–`14` 观测 → `15` HFT"),
]

SKIP_DIRS = {".git", "node_modules", ".cursor", "__pycache__"}
SKIP_FILES = {"renumber-modules-03-14-perf-defer.py"}


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
