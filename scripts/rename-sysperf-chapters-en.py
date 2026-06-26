#!/usr/bin/env python3
"""Rename SysPerf chapter folders to English slugs and update links under 03-SysPerf only."""
from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SYSPERF = ROOT / "03-Systems-Performance-2nd"

# Historical mapping (Chinese slug -> English slug). Idempotent if already renamed.
RENAME_MAP: dict[str, str] = {
    "chapter-01-简介": "chapter-01-intro",
    "chapter-02-方法论": "chapter-02-methodologies",
    "chapter-03-操作系统": "chapter-03-operating-systems",
    "chapter-04-观测工具": "chapter-04-observability-tools",
    "chapter-05-应用程序": "chapter-05-applications",
    "chapter-06-中央处理器": "chapter-06-cpus",
    "chapter-07-内存": "chapter-07-memory",
    "chapter-08-文件系统": "chapter-08-file-systems",
    "chapter-09-磁盘": "chapter-09-disks",
    "chapter-10-网络": "chapter-10-network",
    "chapter-11-云计算": "chapter-11-cloud-computing",
    "chapter-12-基准测试": "chapter-12-benchmarking",
    "chapter-13-perf性能分析": "chapter-13-perf",
    "chapter-14-Ftrace跟踪": "chapter-14-ftrace",
    "chapter-15-BPF技术": "chapter-15-bpf",
    "chapter-16-案例研究": "chapter-16-case-studies",
}


def git_mv(src: Path, dst: Path) -> None:
    subprocess.run(["git", "mv", str(src), str(dst)], cwd=ROOT, check=True)
    print(f"mv {src.name} -> {dst.name}")


def rename_folders() -> None:
    for old, new in RENAME_MAP.items():
        src = SYSPERF / old
        dst = SYSPERF / new
        if not src.exists():
            if dst.exists():
                print(f"skip (already): {new}")
            continue
        if dst.exists():
            raise FileExistsError(dst)
        git_mv(src, dst)


def update_links() -> None:
    pairs = sorted(RENAME_MAP.items(), key=lambda x: len(x[0]), reverse=True)
    changed = 0
    for path in SYSPERF.rglob("*"):
        if not path.is_file() or path.suffix not in {".md", ".py", ".mdc"}:
            continue
        text = path.read_text(encoding="utf-8")
        new_text = text
        for old, new in pairs:
            new_text = new_text.replace(old, new)
        if new_text != text:
            path.write_text(new_text, encoding="utf-8")
            print(f"links {path.relative_to(ROOT)}")
            changed += 1
    print(f"updated {changed} files under SysPerf")


def main() -> None:
    rename_folders()
    update_links()


if __name__ == "__main__":
    main()
