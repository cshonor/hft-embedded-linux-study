# -*- coding: utf-8 -*-
"""Move async_tokio demos into book-aligned section folders."""
from __future__ import annotations

import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

MOVES: list[tuple[str, str]] = [
    # (from_relative, to_relative)
    (
        "ch05_coroutines_generators/5.3-simple-generator/5.1-coroutines-intro-stdlib-demo.rs",
        "ch05_coroutines_generators/5.1-coroutines-intro/5.1-coroutines-intro-stdlib-demo.rs",
    ),
    (
        "ch09_async_design_patterns/9.5-retry-pattern/9.1-isolated-module-stdlib-demo.rs",
        "ch09_async_design_patterns/9.1-building-isolated-module/9.1-building-isolated-module-stdlib-demo.rs",
    ),
    (
        "ch10_dependency_free_async_server/10.2-building-std-async-runtime/10.1-basics-stdlib-demo.rs",
        "ch10_dependency_free_async_server/10.1-setting-up-basics/10.1-setting-up-basics-stdlib-demo.rs",
    ),
    (
        "ch11_async_testing_debugging/11.3-testing-for-deadlocks/11.1-sync-testing-stdlib-demo.rs",
        "ch11_async_testing_debugging/11.1-performing-basic-sync-testing/11.1-performing-basic-sync-testing-stdlib-demo.rs",
    ),
    (
        "ch03_custom_task_queue/3.6-custom-join-macro/3.1-async-queue-stdlib-demo.rs",
        "ch03_custom_task_queue/3.1-async-queue/3.1-async-queue-stdlib-demo.rs",
    ),
    (
        "ch04_network_io_runtime/4.5-mio-poll/4.1-executor-connector-stdlib-demo.rs",
        "ch04_network_io_runtime/4.1-executor-connector/4.1-executor-connector-stdlib-demo.rs",
    ),
]

# Legacy chapter-root duplicates (already copied into section dirs)
REMOVE_IF_EXISTS = [
    "ch01_async_intro/ch01_reqwest_join.rs",
    "ch01_async_intro/demo.rs",
    "ch02_async_rust_core/ch02_counter_future.rs",
    "ch02_async_rust_core/demo.rs",
    "ch03_custom_task_queue/ch03_join_macro_flume.rs",
    "ch03_custom_task_queue/demo.rs",
    "ch04_network_io_runtime/ch04_mio_poll_listener.rs",
    "ch04_network_io_runtime/demo.rs",
    "ch05_coroutines_generators/ch05_resume_state_machine.rs",
    "ch05_coroutines_generators/demo.rs",
    "ch06_reactive_async_streams/ch06_callback.rs",
    "ch06_reactive_async_streams/ch06_heater_display.rs",
    "ch06_reactive_async_streams/demo.rs",
    "ch07_tokio_graceful_shutdown/ch07_ctrl_c_shutdown.rs",
    "ch07_tokio_graceful_shutdown/ch07_local_pool_pinned.rs",
    "ch07_tokio_graceful_shutdown/ch07_runtime_builder_hooks.rs",
    "ch07_tokio_graceful_shutdown/demo.rs",
    "ch08_actor_model/ch08_actor_resp_channel.rs",
    "ch08_actor_model/demo.rs",
    "ch09_async_design_patterns/ch09_retry_backoff.rs",
    "ch09_async_design_patterns/demo.rs",
    "ch10_dependency_free_async_server/ch10_noop_waker_block_on.rs",
    "ch10_dependency_free_async_server/ch10_std_tcp_nonblocking.rs",
    "ch10_dependency_free_async_server/demo.rs",
    "ch11_async_testing_debugging/ch11_timeout_deadlock_probe.rs",
    "ch11_async_testing_debugging/demo.rs",
]


def main() -> None:
    for src_rel, dst_rel in MOVES:
        src = ROOT / src_rel
        dst = ROOT / dst_rel
        if not src.exists():
            print("skip (missing):", src_rel)
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        if dst.exists():
            print("skip (exists):", dst_rel)
            src.unlink()
        else:
            shutil.move(str(src), str(dst))
            print("moved:", src_rel, "->", dst_rel)

    for rel in REMOVE_IF_EXISTS:
        p = ROOT / rel
        if p.exists():
            p.unlink()
            print("removed duplicate:", rel)

    print("Done.")


if __name__ == "__main__":
    main()
