# -*- coding: utf-8 -*-
"""为 02-async_tokio 生成 / 更新 Cargo.toml：把散装 `.rs` 全部注册为 `[[bin]]`。

为什么需要这个脚本
------------------
小节 demo 的文件名形如 `1.1-what-is-async-join-demo.rs`，**含 `.`**，
rustc 不接受它作为 crate 名（`invalid character '.' in crate name`）。
早年的土办法是在每个文件顶部加 `#![crate_name = "..."]`，但那与 Cargo 的
`--crate-name` 冲突。正确解法是**不改名**，改用 `[[bin]]` 显式声明：

    [[bin]]
    name = "c1_1_what_is_async_join_demo"          # 合法化的名字
    path = "ch01_async_intro/1.1-what-is-async/code/1.1-what-is-async-join-demo.rs"

新增 demo 后重跑本脚本即可，无需手工维护 Cargo.toml。

    python 05-Async-Concurrency-Network/scripts/gen_async_cargo.py
"""
import re
from pathlib import Path

# 本脚本位于 <repo>/17-rust-foundation/05-Async-Concurrency-Network/scripts/
ROOT = Path(__file__).resolve().parent.parent / "02-async_tokio"

MAIN_RE = re.compile(r"fn\s+main\s*\(|#\[(?:tokio|async_std|actix_rt)::main\]")


def legal(name: str) -> str:
    """把文件名转成合法的 crate/bin 名：非字母数字 → _，数字开头补 c。"""
    n = re.sub(r"[^0-9a-zA-Z_]", "_", name)
    n = re.sub(r"_+", "_", n).strip("_")
    if not n or n[0].isdigit():
        n = "c" + n
    return n


def main() -> None:
    # 排除 target/ —— 依赖的构建产物里也有 .rs（如 serde 生成的 private.rs）
    files = sorted(
        f for f in ROOT.rglob("*.rs")
        if "target" not in f.relative_to(ROOT).parts
    )
    with_main = [f for f in files if MAIN_RE.search(f.read_text(encoding="utf-8", errors="replace"))]
    skipped = [f for f in files if f not in with_main]

    used, entries = set(), []
    for f in with_main:
        base = legal(f.stem)
        name, i = base, 2
        while name in used:          # 重名时加后缀，保证唯一
            name = f"{base}_{i}"
            i += 1
        used.add(name)
        entries.append((name, f.relative_to(ROOT).as_posix()))

    toml = [
        "[package]",
        'name = "async_tokio_lab"',
        'version = "0.1.0"',
        'edition = "2021"',
        "publish = false",
        "",
        "# 本文件由 scripts/gen_async_cargo.py 生成，新增 demo 后重跑该脚本即可。",
        "# 原文件命名为 X.Y-name-demo.rs（含 '.'），无法直接作为 crate 名，",
        "# 故用 [[bin]] 显式给出合法 name + 指向原文件的 path，原文件不改名。",
        "",
        "[dependencies]",
        'tokio = { version = "1", features = ["full"] }',
        'tokio-util = { version = "0.7", features = ["rt"] }  # ch07/7.2 LocalPoolHandle',
        'mio = { version = "1", features = ["net", "os-poll", "os-ext"] }',
        'reqwest = { version = "0.12", features = ["json", "rustls-tls"], default-features = false }',
        'futures-lite = "2"   # ch03/3.6 join! 宏里的 block_on',
        'flume = "0.11"       # ch03 自定义任务队列的通道',
        "",
        "[workspace]  # 独立工程，不并入上级 workspace",
        "",
    ]
    for name, path in entries:
        toml += ["[[bin]]", f'name = "{name}"', f'path = "{path}"', ""]

    (ROOT / "Cargo.toml").write_text("\n".join(toml), encoding="utf-8")

    print(f"已生成 {ROOT / 'Cargo.toml'}")
    print(f"  [[bin]] 条目: {len(entries)} / 扫描到 .rs: {len(files)}")
    for f in skipped:
        print(f"  [跳过·无 main] {f.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
