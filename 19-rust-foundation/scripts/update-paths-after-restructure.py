#!/usr/bin/env python3
"""Update repo paths after 04-Async + 05-Compilers restructure."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ASYNC_ROOT = "05-Async-Concurrency-Network"
EXTS = {".md", ".gitignore", ".toml", ".rs", ".py", ".ps1"}

SKIP_DIRS = {".git", "target", ".cursor"}


def should_process(path: Path) -> bool:
    if path.suffix not in EXTS:
        return False
    return not any(part in SKIP_DIRS for part in path.parts)


def rel_to_async(path: Path) -> bool:
    try:
        path.relative_to(ROOT / ASYNC_ROOT)
        return True
    except ValueError:
        return False


def transform(content: str, path: Path) -> str:
    inside_async = rel_to_async(path)

    # Compilers section renumber
    content = content.replace(
        "04_Compilers-and-LLVM-Learning", "06_Compilers-and-LLVM-Learning"
    )

    # Repo-root cargo / manifest paths (all files)
    content = content.replace(
        "--manifest-path rust_network_programming/",
        f"--manifest-path {ASYNC_ROOT}/rust_network_programming/",
    )
    content = content.replace(
        "--manifest-path 04_Compilers-and-LLVM-Learning/",
        "--manifest-path 06_Compilers-and-LLVM-Learning/",
    )
    content = content.replace(
        "04_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/",
        "06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/",
    )

    if inside_async:
        # Scripts invoked from repo root
        content = content.replace(
            "python async_tokio/scripts/",
            f"python {ASYNC_ROOT}/async_tokio/scripts/",
        )
        return content

    # Outside 04-Async: update top-level track references
    replacements = [
        (r"\]\(atomic/", f"]({ASYNC_ROOT}/atomic/"),
        (r"\]\(async_tokio/", f"]({ASYNC_ROOT}/async_tokio/"),
        (r"\]\(rust_network_programming/", f"]({ASYNC_ROOT}/rust_network_programming/"),
        (r"\]\(\.\./\.\./atomic/", f"](../../{ASYNC_ROOT}/atomic/"),
        (r"\]\(\.\./\.\./async_tokio/", f"](../../{ASYNC_ROOT}/async_tokio/"),
        (r"\]\(\.\./\.\./rust_network_programming/", f"](../../{ASYNC_ROOT}/rust_network_programming/"),
        (r"\]\(\.\./atomic/", f"](../{ASYNC_ROOT}/atomic/"),
        (r"\]\(\.\./async_tokio/", f"](../{ASYNC_ROOT}/async_tokio/"),
        (r"\]\(\.\./rust_network_programming/", f"](../{ASYNC_ROOT}/rust_network_programming/"),
        (r"`atomic/`", f"`{ASYNC_ROOT}/atomic/`"),
        (r"`async_tokio/`", f"`{ASYNC_ROOT}/async_tokio/`"),
        (r"`rust_network_programming/`", f"`{ASYNC_ROOT}/rust_network_programming/`"),
        (r"cd async_tokio/", f"cd {ASYNC_ROOT}/async_tokio/"),
        (r"cd atomic/", f"cd {ASYNC_ROOT}/atomic/"),
        (r"python async_tokio/", f"python {ASYNC_ROOT}/async_tokio/"),
    ]
    for old, new in replacements:
        content = re.sub(old, new, content)

    return content


def main() -> None:
    changed = 0
    for path in ROOT.rglob("*"):
        if not path.is_file() or not should_process(path):
            continue
        if path.name == Path(__file__).name:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        new_text = transform(text, path)
        if new_text != text:
            path.write_text(new_text, encoding="utf-8", newline="\n")
            changed += 1
    print(f"Updated {changed} files")


if __name__ == "__main__":
    main()
