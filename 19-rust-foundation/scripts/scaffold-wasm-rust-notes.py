#!/usr/bin/env python3
"""Scaffold per-section notes for 07-Programming-WebAssembly-with-Rust."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "07-Programming-WebAssembly-with-Rust"

CHAPTERS: dict[str, tuple[int, list[tuple[str, str, str]]]] = {
    "chapter01_wasm_fundamentals": (
        1,
        [
            ("1.1", "introducing-wasm", "Introducing WebAssembly"),
            ("1.2", "wasm-architecture", "Understanding WebAssembly Architecture"),
            ("1.3", "building-wasm-app", "Building a WebAssembly Application"),
            ("1.4", "wrap-up", "Wrapping Up"),
        ],
    ),
    "chapter02_wasm_checkers": (
        2,
        [
            ("2.1", "checkers-board-game", "Playing Checkers, the Board Game"),
            ("2.2", "data-structure-constraints", "Coping with Data Structure Constraints"),
            ("2.3", "game-rules", "Implementing Game Rules"),
            ("2.4", "moving-players", "Moving Players"),
            ("2.5", "testing-wasm-checkers", "Testing Wasm Checkers"),
            ("2.6", "wrap-up", "Wrapping Up"),
        ],
    ),
    "chapter03_rust_wasm": (
        3,
        [
            ("3.1", "introducing-rust", "Introducing Rust"),
            ("3.2", "installing-rust", "Installing Rust"),
            ("3.3", "hello-wasm-rust", "Building Hello WebAssembly in Rust"),
            ("3.4", "rusty-checkers", "Creating Rusty Checkers"),
            ("3.5", "rusty-checkers-ffi", "Coding the Rusty Checkers WebAssembly Interface"),
            ("3.6", "rusty-checkers-js", "Playing Rusty Checkers in JavaScript"),
            ("3.7", "wrap-up", "Wrapping Up"),
        ],
    ),
    "chapter04_js_integration": (
        4,
        [
            ("4.1", "better-hello-world", "Creating a Better Hello, World"),
            ("4.2", "rogue-wasm-game", "Building the Rogue WebAssembly Game"),
            ("4.3", "experimenting-further", "Experimenting Further"),
            ("4.4", "wrap-up", "Wrapping Up"),
        ],
    ),
    "chapter05_yew": (
        5,
        [
            ("5.1", "getting-started-yew", "Getting Started with Yew"),
            ("5.2", "live-chat-app", "Building a Live Chat Application"),
            ("5.3", "wrap-up", "Wrapping Up"),
        ],
    ),
    "chapter06_nonweb_hosts": (
        6,
        [
            ("6.1", "good-host", "How to Be a Good Host"),
            ("6.2", "interpreting-wasm-rust", "Interpreting WebAssembly Modules with Rust"),
            ("6.3", "console-checkers", "Building a Console Host Checkers Player"),
            ("6.4", "wrap-up", "Wrapping Up"),
        ],
    ),
    "chapter07_iot": (
        7,
        [
            ("7.1", "indicator-overview", "Overview of the Generic Indicator Module"),
            ("7.2", "creating-indicators", "Creating Indicator Modules"),
            ("7.3", "rust-arm", "Building Rust Applications for ARM Devices"),
            ("7.4", "raspberry-pi-host", "Hosting Indicator Modules on a Raspberry Pi"),
            ("7.5", "hardware-list", "Hardware Shopping List"),
            ("7.6", "endless-possibilities", "Endless Possibilities"),
            ("7.7", "wrap-up", "Wrapping Up"),
        ],
    ),
    "chapter08_waros": (
        8,
        [
            ("8.1", "homage-crobots", "An Homage to Crobots"),
            ("8.2", "waros-api", "Designing the WARoS API"),
            ("8.3", "match-engine", "Building the WARoS Match Engine"),
            ("8.4", "wasm-robots", "Creating WebAssembly Robots"),
            ("8.5", "robots-cloud", "Robots in the Cloud"),
            ("8.6", "wrap-up", "Wrapping Up"),
            ("8.7", "conclusion", "Conclusion"),
        ],
    ),
    "appendix": (
        0,
        [
            ("A1", "serverless", "WebAssembly and Serverless"),
            ("A2", "security", "Securing WebAssembly Modules"),
        ],
    ),
}

CHAPTER_TITLES = {
    "chapter01_wasm_fundamentals": "WebAssembly Fundamentals",
    "chapter02_wasm_checkers": "Building WebAssembly Checkers",
    "chapter03_rust_wasm": "Wading into WebAssembly with Rust",
    "chapter04_js_integration": "Integrating WebAssembly with JavaScript",
    "chapter05_yew": "Advanced JavaScript Integration with Yew",
    "chapter06_nonweb_hosts": "Hosting Modules Outside the Browser",
    "chapter07_iot": "Exploring the Internet of WebAssembly Things",
    "chapter08_waros": "Building WARoS — The WebAssembly Robot System",
    "appendix": "附录",
}


def fn(sid: str, suffix: str) -> str:
    return f"{sid}-{suffix}.md"


def stub(sid: str, title: str, ch_num: int, prev_f: str | None, next_f: str | None) -> str:
    idx = "附录" if ch_num == 0 else f"第 {ch_num} 章"
    nav = [f"> {idx}索引：[README](./README.md)"]
    if prev_f:
        nav.append(f"前：[{prev_f.replace('.md', '')}](./{prev_f})")
    if next_f:
        nav.append(f"后：[{next_f.replace('.md', '')}](./{next_f})")
    return f"""# {sid} {title}

{" · ".join(nav)}

---

## 一句话

（待补充）

---

## 原书要点

（阅读原书后填写）

---

## WAT / 工具 / 代码锚点

| 项 | 说明 |
|----|------|
| | （wasm-pack · wasm-bindgen · WAT 片段 · demo 路径） |

---

## 我的笔记

（刷书、跑 demo、与 JS/宿主对照后在此迭代）

---

## 相关

- 
"""


def update_readme(chapter_dir: Path, ch_num: int, entries: list[tuple[str, str, str]]) -> None:
    title = CHAPTER_TITLES.get(chapter_dir.name, chapter_dir.name)
    readme = chapter_dir / "README.md"
    rows = "\n".join(
        f"| **{sid}** | {title_en} | [{fn(sid, suf)}](./{fn(sid, suf)}) |"
        for sid, suf, title_en in entries
    )
    ch_label = "附录" if ch_num == 0 else f"第 {ch_num} 章"
    body = f"""# {ch_label} · {title}

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md)

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
{rows}

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。
"""
    readme.write_text(body, encoding="utf-8")


def main() -> None:
    for folder, (ch_num, entries) in CHAPTERS.items():
        chapter_dir = ROOT / folder
        chapter_dir.mkdir(parents=True, exist_ok=True)
        files = [fn(sid, suf) for sid, suf, _ in entries]
        for i, (sid, suffix, title) in enumerate(entries):
            path = chapter_dir / fn(sid, suffix)
            path.write_text(
                stub(sid, title, ch_num, files[i - 1] if i else None, files[i + 1] if i + 1 < len(files) else None),
                encoding="utf-8",
            )
        update_readme(chapter_dir, ch_num, entries)
    print("Scaffolded 07-Programming-WebAssembly-with-Rust")


if __name__ == "__main__":
    main()
