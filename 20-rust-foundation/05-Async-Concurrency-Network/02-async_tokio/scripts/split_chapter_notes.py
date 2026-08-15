# -*- coding: utf-8 -*-
"""Split async_tokio 本章学习笔记.md into per-section files (full content)."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

PATH_REPLACEMENTS = [
    ("ch01_reqwest_join.rs", "1.6-improving-http-request-performance/1.6-improving-http-request-performance-demo.rs"),
    ("1.6-http-performance/", "1.6-improving-http-request-performance/"),
    ("ch02_counter_future.rs", "2.2-futures/2.2-futures-counter-demo.rs"),
    ("2.1-future-trait/", "2.2-futures/"),
    ("2.1-future-trait-counter-demo.rs", "2.2-futures-counter-demo.rs"),
    ("ch03_join_macro_flume.rs", "3.6-creating-join-macro/3.6-creating-join-macro-demo.rs"),
    ("3.6-custom-join-macro/", "3.6-creating-join-macro/"),
    ("3.1-async-queue/", "3.1-building-async-queue/"),
    ("ch04_mio_poll_listener.rs", "4.8-polling-sockets-in-futures/4.8-polling-sockets-listener-demo.rs"),
    ("4.5-mio-poll/", "4.8-polling-sockets-in-futures/"),
    ("4.5-mio-poll-listener-demo.rs", "4.8-polling-sockets-listener-demo.rs"),
    ("ch05_resume_state_machine.rs", "5.2-generating-with-coroutines/5.2-generating-state-machine-demo.rs"),
    ("5.3-simple-generator/", "5.2-generating-with-coroutines/"),
    ("5.3-simple-generator-state-machine-demo.rs", "5.2-generating-state-machine-demo.rs"),
    ("ch06_callback.rs", "6.4-user-input-via-callbacks/6.4-user-input-via-callbacks-demo.rs"),
    ("ch06_heater_display.rs", "6.2-building-display-observer/6.2-building-display-observer-demo.rs"),
    ("6.1-heater-system/", "6.1-basic-reactive-system-and-subjects/"),
    ("6.1-heater-system-callback-demo.rs", "6.4-user-input-via-callbacks-demo.rs"),
    ("6.1-heater-system-display-demo.rs", "6.2-building-display-observer-demo.rs"),
    ("ch07_local_pool_pinned.rs", "7.2-processing-tasks-with-local-pools/7.2-processing-tasks-with-local-pools-demo.rs"),
    ("ch07_runtime_builder_hooks.rs", "7.1-building-a-runtime/7.1-building-a-runtime-demo.rs"),
    ("ch07_ctrl_c_shutdown.rs", "7.4-graceful-shutdowns/7.4-graceful-shutdowns-ctrl-c-demo.rs"),
    ("7.1-runtime-builder/", "7.1-building-a-runtime/"),
    ("7.2-local-pool/", "7.2-processing-tasks-with-local-pools/"),
    ("7.4-graceful-shutdown/", "7.4-graceful-shutdowns/"),
    ("ch08_actor_resp_channel.rs", "8.1-building-basic-actor/8.1-building-basic-actor-demo.rs"),
    ("8.1-custom-actor/", "8.1-building-basic-actor/"),
    ("ch09_retry_backoff.rs", "9.5-retry-pattern/9.5-retry-pattern-backoff-demo.rs"),
    ("9.1-isolated-module/", "9.1-building-isolated-module/"),
    ("10.1-basics/", "10.1-setting-up-basics/"),
    ("10.2-std-runtime/", "10.2-building-std-async-runtime/"),
    ("10.2-std-runtime-noop-waker-demo.rs", "10.2-building-std-async-runtime-noop-waker-demo.rs"),
    ("10.2-std-runtime-tcp-nonblocking-demo.rs", "10.2-building-std-async-runtime-tcp-nonblocking-demo.rs"),
    ("ch10_std_tcp_nonblocking.rs", "10.2-building-std-async-runtime/10.2-building-std-async-runtime-tcp-nonblocking-demo.rs"),
    ("ch10_noop_waker_block_on.rs", "10.2-building-std-async-runtime/10.2-building-std-async-runtime-noop-waker-demo.rs"),
    ("ch11_timeout_deadlock_probe.rs", "11.3-testing-for-deadlocks/11.3-testing-for-deadlocks-timeout-demo.rs"),
    ("11.1-sync-testing/", "11.1-performing-basic-sync-testing/"),
    ("11.3-deadlock-probe/", "11.3-testing-for-deadlocks/"),
    ("11.4-future-polling.md", "11.7-fine-grained-future-testing.md"),
    ("同目录 ``demo.rs``", "各小节 `X.Y-slug/` 下 `*-stdlib-demo.rs`"),
    ("本目录 `ch10_std_tcp_nonblocking.rs`", "`10.2-building-std-async-runtime/10.2-building-std-async-runtime-tcp-nonblocking-demo.rs`"),
    ("本目录 `ch10_noop_waker_block_on.rs`", "`10.2-building-std-async-runtime/10.2-building-std-async-runtime-noop-waker-demo.rs`"),
    ("3.6-custom-join-macro/3.1-async-queue-stdlib-demo.rs", "3.1-async-queue/3.1-async-queue-stdlib-demo.rs"),
    ("4.5-mio-poll/4.1-executor-connector-stdlib-demo.rs", "4.1-executor-connector/4.1-executor-connector-stdlib-demo.rs"),
    ("5.3-simple-generator/5.1-coroutines-intro-stdlib-demo.rs", "5.1-coroutines-intro/5.1-coroutines-intro-stdlib-demo.rs"),
    ("9.5-retry-pattern/9.1-isolated-module-stdlib-demo.rs", "9.1-building-isolated-module/9.1-building-isolated-module-stdlib-demo.rs"),
    ("10.2-std-runtime/10.1-basics-stdlib-demo.rs", "10.1-setting-up-basics/10.1-setting-up-basics-stdlib-demo.rs"),
    ("11.3-deadlock-probe/11.1-sync-testing-stdlib-demo.rs", "11.1-performing-basic-sync-testing/11.1-performing-basic-sync-testing-stdlib-demo.rs"),
]


def fix_paths(text: str) -> str:
    for old, new in PATH_REPLACEMENTS:
        text = text.replace(old, new)
    return text


def split_by_h2(src: str) -> dict[str, str]:
    parts: dict[str, str] = {}
    key = "_preamble"
    lines: list[str] = []
    for line in src.splitlines(keepends=True):
        if line.startswith("## ") and not line.startswith("### "):
            parts[key] = "".join(lines)
            key = line[3:].strip()
            lines = [line]
        else:
            lines.append(line)
    parts[key] = "".join(lines)
    return parts


def section_slice(text: str, start: str, end: str | None = None) -> str:
    if start not in text:
        return ""
    i = text.index(start)
    chunk = text[i:]
    if end:
        rest = chunk[len(start) :]
        if end in rest:
            chunk = chunk[: len(start) + rest.index(end)]
    return chunk


def write_section(ch: Path, fname: str, chunks: tuple[str, ...]) -> None:
    body = fix_paths("".join(chunks)).strip()
    if not body:
        return
    if not body.startswith("#"):
        title = fname.replace(".md", "").split("-", 1)[-1].replace("-", " ").title()
        body = f"# {title}\n\n{body}\n"
    footer = (
        "\n---\n\n"
        f"> 章节索引：[本章学习笔记.md](./本章学习笔记.md) · "
        "[本书详细目录](../本书详细目录.md)\n"
    )
    (ch / fname).write_text(body + footer, encoding="utf-8")
    print("Wrote", ch.name, fname)


def write_index(ch: Path, title: str, rows: list[tuple[str, str, str]]) -> None:
    lines = [
        f"# {title}\n",
        "> **正文已拆至各小节文件**（非占位）。书目：[本书详细目录.md](../本书详细目录.md)\n",
        "| 书 § | 笔记 | Demo |",
        "|------|------|------|",
    ]
    for sec, note, demo in rows:
        demo_cell = f"[{demo}/](./{demo}/)" if demo else "—"
        lines.append(f"| {sec} | [{note}](./{note}) | {demo_cell} |")
    lines.append("")
    (ch / "本章学习笔记.md").write_text("\n".join(lines), encoding="utf-8")
    print("index", ch.name)


def split_ch01():
    ch = ROOT / "ch01_async_intro"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "1.7-summary.md" in src and "共 7 节" in src:
        print("Ch01 already on 7-section layout, skip")
        return
    print("Ch01: maintain 1.1..1.7 md manually")


def split_ch02():
    """Ch2 uses 10 book sections; index-only 本章学习笔记 — skip overwrite if already split."""
    ch = ROOT / "ch02_async_rust_core"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "2.10-summary.md" in src and "共 10 节" in src:
        print("Ch02 already on 10-section layout, skip")
        return
    print("Ch02: maintain 2.1..2.10 md manually; see 本章学习笔记.md index")


def split_ch03():
    ch = ROOT / "ch03_custom_task_queue"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "3.9-summary.md" in src and "共 9 节" in src:
        print("Ch03 already on 9-section layout, skip")
        return
    print("Ch03: maintain 3.1..3.9 md manually")


def split_ch04():
    ch = ROOT / "ch04_network_io_runtime"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "4.10-summary.md" in src and "共 10 节" in src:
        print("Ch04 already on 10-section layout, skip")
        return
    print("Ch04: maintain 4.1..4.10 md manually")


def split_ch05():
    ch = ROOT / "ch05_coroutines_generators"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "5.6-summary.md" in src and "共 6 节" in src:
        print("Ch05 already on 6-section layout, skip")
        return
    print("Ch05: maintain 5.1..5.6 md manually")


def split_ch06():
    ch = ROOT / "ch06_reactive_async_streams"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "6.8-summary.md" in src and "共 8 节" in src:
        print("Ch06 already on 8-section layout, skip")
        return
    print("Ch06: maintain 6.1..6.8 md manually")


def split_ch07():
    ch = ROOT / "ch07_tokio_graceful_shutdown"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "7.5-summary.md" in src and "共 5 节" in src:
        print("Ch07 already on 5-section layout, skip")
        return
    print("Ch07: maintain 7.1..7.5 md manually")


def split_ch08():
    ch = ROOT / "ch08_actor_model"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "8.6-summary.md" in src and "共 6 节" in src:
        print("Ch08 already on 6-section layout, skip")
        return
    print("Ch08: maintain 8.1..8.6 md manually")


def split_ch09():
    ch = ROOT / "ch09_async_design_patterns"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "9.7-summary.md" in src and "共 7 节" in src:
        print("Ch09 already on 7-section layout, skip")
        return
    print("Ch09: maintain 9.1..9.7 md manually")


def split_ch10():
    ch = ROOT / "ch10_dependency_free_async_server"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "10.5-summary.md" in src and "共 5 节" in src:
        print("Ch10 already on 5-section layout, skip")
        return
    print("Ch10: maintain 10.1..10.5 md manually")


def split_ch11():
    ch = ROOT / "ch11_async_testing_debugging"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "11.8-summary.md" in src and "共 8 节" in src:
        print("Ch11 already on 8-section layout, skip")
        return
    print("Ch11: maintain 11.1..11.8 md manually")


def main() -> None:
    split_ch01()
    split_ch02()
    split_ch03()
    split_ch04()
    split_ch05()
    split_ch06()
    split_ch07()
    split_ch08()
    split_ch09()
    split_ch10()
    split_ch11()
    print("Done.")


if __name__ == "__main__":
    main()
