#!/usr/bin/env python3
"""Fix doubled section prefixes in intra-chapter links after ch04-10 restructure."""
from __future__ import annotations

import re
from pathlib import Path

ATOMIC = Path(__file__).resolve().parents[1]
MANIFEST = "05-Async-Concurrency-Network/01-atomic/Cargo.toml"

CHAPTER_DIRS = [
    "Chapter-04-Spin-Locks",
    "Chapter-05-Channels",
    "Chapter-06-Custom-Arc",
    "Chapter-07-Processors",
    "Chapter-08-OS-Primitives",
    "Chapter-09-Custom-Locks",
    "Chapter-10-Advanced-Concurrent-Data-Structures",
]

DEMO_FIXES = {
    "4.1-minimal-implementation/4.1-minimal-implementation-demo.rs": "4.1-minimal-implementation/code/4.1-minimal-implementation-demo.rs",
    "5.1-mutex-based-channel/5.1-mutex-based-channel-demo.rs": "5.1-mutex-based-channel/code/5.1-mutex-based-channel-demo.rs",
}


def section_slugs(ch: Path) -> set[str]:
    return {p.name for p in ch.iterdir() if p.is_dir() and re.match(r"\d+\.\d+-", p.name)}


def fix_in_section(content: str, section: str, all_sections: set[str]) -> str:
    # same-section: ./section/foo -> ./foo
    prefix = f"./{section}/"
    content = content.replace(prefix, "./")
    # cross-section from inside section: ./other-section/ -> ../other-section/
    for other in all_sections:
        if other == section:
            continue
        content = content.replace(f"](./{other}/", f"](../{other}/")
        content = content.replace(f"](({other}/", f"](../{other}/")
    return content


def fix_chapter_root(content: str, all_sections: set[str]) -> str:
    for sec in all_sections:
        content = content.replace(f"](./{sec}/{sec}.md)", f"](./{sec}/{sec}.md)")  # already ok
    return content


def fix_demo_refs(content: str) -> str:
    for old, new in DEMO_FIXES.items():
        content = content.replace(old, new)
    content = content.replace(
        "Chapter-04-Spin-Locks/4.1-minimal-implementation/",
        "Chapter-04-Spin-Locks/4.1-minimal-implementation/code/",
    )
    # only when pointing at demo rs, not md - be careful
    content = content.replace(
        "../Chapter-04-Spin-Locks/4.1-minimal-implementation/4.1-minimal-implementation-demo.rs",
        "../Chapter-04-Spin-Locks/4.1-minimal-implementation/code/4.1-minimal-implementation-demo.rs",
    )
    return content


def main() -> None:
    for ch_name in CHAPTER_DIRS:
        ch = ATOMIC / ch_name
        if not ch.is_dir():
            continue
        sections = section_slugs(ch)
        for md in ch.rglob("*.md"):
            rel = md.relative_to(ch)
            t = md.read_text(encoding="utf-8")
            orig = t
            if len(rel.parts) >= 2 and rel.parts[0] in sections:
                t = fix_in_section(t, rel.parts[0], sections)
            t = fix_demo_refs(t)
            t = t.replace("`atomic/Cargo.toml`", f"`{MANIFEST}`")
            if t != orig:
                md.write_text(t, encoding="utf-8", newline="\n")
                print("fixed", md.relative_to(ATOMIC))

    # 章节与小节对照表 demo paths
    table = ATOMIC / "章节与小节对照表.md"
    t = table.read_text(encoding="utf-8")
    replacements = [
        ("[4.1-minimal-implementation/](./Chapter-04-Spin-Locks/4.1-minimal-implementation/)", "[4.1-minimal-implementation/code/](./Chapter-04-Spin-Locks/4.1-minimal-implementation/code/)"),
        ("| 4.1–4.3 | [4.1-minimal-implementation.md](./Chapter-04-Spin-Locks/4.1-minimal-implementation/4.1-minimal-implementation.md) 等 | [4.1-minimal-implementation/](./Chapter-04-Spin-Locks/4.1-minimal-implementation/) |", "| 4.1 | [4.1-minimal-implementation.md](./Chapter-04-Spin-Locks/4.1-minimal-implementation/4.1-minimal-implementation.md) | [4.1-minimal-implementation/code/](./Chapter-04-Spin-Locks/4.1-minimal-implementation/code/) |\n| 4.2 | [4.2-unsafe-spin-lock.md](./Chapter-04-Spin-Locks/4.2-unsafe-spin-lock/4.2-unsafe-spin-lock.md) | ↑ demo |\n| 4.3 | [4.3-safe-lock-guard.md](./Chapter-04-Spin-Locks/4.3-safe-lock-guard/4.3-safe-lock-guard.md) | ↑ |\n| 4.4 | [4.4-summary.md](./Chapter-04-Spin-Locks/4.4-summary/4.4-summary.md) | — |"),
        ("[5.1-mutex-based-channel/](./Chapter-05-Channels/5.1-mutex-based-channel/)", "[5.1-mutex-based-channel/code/](./Chapter-05-Channels/5.1-mutex-based-channel/code/)"),
        ("[5.2-unsafe-one-shot-channel/](./Chapter-05-Channels/5.2-unsafe-one-shot-channel/)", "[5.2-unsafe-one-shot-channel/code/](./Chapter-05-Channels/5.2-unsafe-one-shot-channel/code/)"),
        ("| 5.3–5.7 | `5.3` … `5.7` 笔记 md | 5.3–5.6 以笔记为主；阻塞语义见 5.2 demo |", "| 5.3 | [5.3-runtime-checks-safety.md](./Chapter-05-Channels/5.3-runtime-checks-safety/5.3-runtime-checks-safety.md) | — |\n| 5.4 | [5.4-types-safety.md](./Chapter-05-Channels/5.4-types-safety/5.4-types-safety.md) | — |\n| 5.5 | [5.5-borrowing-avoid-allocation.md](./Chapter-05-Channels/5.5-borrowing-avoid-allocation/5.5-borrowing-avoid-allocation.md) | — |\n| 5.6 | [5.6-blocking.md](./Chapter-05-Channels/5.6-blocking/5.6-blocking.md) | 见 5.2 code |\n| 5.7 | [5.7-summary.md](./Chapter-05-Channels/5.7-summary/5.7-summary.md) | — |"),
        ("[6.1-basic-reference-counting/](./Chapter-06-Custom-Arc/6.1-basic-reference-counting/)", "[6.1-basic-reference-counting/code/](./Chapter-06-Custom-Arc/6.1-basic-reference-counting/code/)"),
        ("[6.2-testing-it/](./Chapter-06-Custom-Arc/6.2-testing-it/)", "[6.2-testing-it/code/](./Chapter-06-Custom-Arc/6.2-testing-it/code/)"),
        ("[6.3-mutation/](./Chapter-06-Custom-Arc/6.3-mutation/)", "[6.3-mutation/code/](./Chapter-06-Custom-Arc/6.3-mutation/code/)"),
        ("[6.4-weak-pointers/](./Chapter-06-Custom-Arc/6.4-weak-pointers/)", "[6.4-weak-pointers/code/](./Chapter-06-Custom-Arc/6.4-weak-pointers/code/)"),
        ("[6.6-optimizing/](./Chapter-06-Custom-Arc/6.6-optimizing/)", "[6.6-optimizing/code/](./Chapter-06-Custom-Arc/6.6-optimizing/code/)"),
        ("[7.2-caching/](./Chapter-07-Processors/7.2-caching/)", "[7.2-caching/code/](./Chapter-07-Processors/7.2-caching/code/)"),
        ("[7.3-reordering/](./Chapter-07-Processors/7.3-reordering/)", "[7.3-reordering/code/](./Chapter-07-Processors/7.3-reordering/code/)"),
        ("[7.1-processor-instructions.md](./Chapter-07-Processors/7.1-processor-instructions.md)", "[7.1-processor-instructions.md](./Chapter-07-Processors/7.1-processor-instructions/7.1-processor-instructions.md)"),
        ("[7.2-caching.md](./Chapter-07-Processors/7.2-caching.md)", "[7.2-caching.md](./Chapter-07-Processors/7.2-caching/7.2-caching.md)"),
        ("[7.3-reordering.md](./Chapter-07-Processors/7.3-reordering.md)", "[7.3-reordering.md](./Chapter-07-Processors/7.3-reordering/7.3-reordering.md)"),
        ("[7.4-memory-fences.md](./Chapter-07-Processors/7.4-memory-fences.md)", "[7.4-memory-fences.md](./Chapter-07-Processors/7.4-memory-fences/7.4-memory-fences.md)"),
        ("[7.5-summary.md](./Chapter-07-Processors/7.5-summary.md)", "[7.5-summary.md](./Chapter-07-Processors/7.5-summary/7.5-summary.md)"),
        ("[8.1-futex.md](./Chapter-08-OS-Primitives/8.1-futex.md)", "[8.1-futex.md](./Chapter-08-OS-Primitives/8.1-futex/8.1-futex.md)"),
        ("[8.1.1-futex-two-phase.md](./Chapter-08-OS-Primitives/8.1.1-futex-two-phase.md)", "[8.1.1-futex-two-phase.md](./Chapter-08-OS-Primitives/8.1-futex/8.1.1-futex-two-phase.md)"),
        ("[8.1.2-futex-wait.md](./Chapter-08-OS-Primitives/8.1.2-futex-wait.md)", "[8.1.2-futex-wait.md](./Chapter-08-OS-Primitives/8.1-futex/8.1.2-futex-wait.md)"),
        ("[8.1.3-futex-wake.md](./Chapter-08-OS-Primitives/8.1.3-futex-wake.md)", "[8.1.3-futex-wake.md](./Chapter-08-OS-Primitives/8.1-futex/8.1.3-futex-wake.md)"),
        ("[8.1.4-futex-requeue-shared.md](./Chapter-08-OS-Primitives/8.1.4-futex-requeue-shared.md)", "[8.1.4-futex-requeue-shared.md](./Chapter-08-OS-Primitives/8.1-futex/8.1.4-futex-requeue-shared.md)"),
        ("[8.2-thread-parks.md](./Chapter-08-OS-Primitives/8.2-thread-parks.md)", "[8.2-thread-parks.md](./Chapter-08-OS-Primitives/8.2-thread-parks/8.2-thread-parks.md)"),
        ("[8.2.1-park-unpark-semantics.md](./Chapter-08-OS-Primitives/8.2.1-park-unpark-semantics.md)", "[8.2.1-park-unpark-semantics.md](./Chapter-08-OS-Primitives/8.2-thread-parks/8.2.1-park-unpark-semantics.md)"),
        ("[8.2.2-park-vs-futex-condvar.md](./Chapter-08-OS-Primitives/8.2.2-park-vs-futex-condvar.md)", "[8.2.2-park-vs-futex-condvar.md](./Chapter-08-OS-Primitives/8.2-thread-parks/8.2.2-park-vs-futex-condvar.md)"),
        ("[8.3-condition-variables.md](./Chapter-08-OS-Primitives/8.3-condition-variables.md)", "[8.3-condition-variables.md](./Chapter-08-OS-Primitives/8.3-condition-variables/8.3-condition-variables.md)"),
        ("[8.3.1-pthread-condvar.md](./Chapter-08-OS-Primitives/8.3.1-pthread-condvar.md)", "[8.3.1-pthread-condvar.md](./Chapter-08-OS-Primitives/8.3-condition-variables/8.3.1-pthread-condvar.md)"),
        ("[8.3.2-windows-condvar.md](./Chapter-08-OS-Primitives/8.3.2-windows-condvar.md)", "[8.3.2-windows-condvar.md](./Chapter-08-OS-Primitives/8.3-condition-variables/8.3.2-windows-condvar.md)"),
        ("[8.3.3-rust-os-mapping.md](./Chapter-08-OS-Primitives/8.3.3-rust-os-mapping.md)", "[8.3.3-rust-os-mapping.md](./Chapter-08-OS-Primitives/8.3-condition-variables/8.3.3-rust-os-mapping.md)"),
        ("[8.4-summary.md](./Chapter-08-OS-Primitives/8.4-summary.md)", "[8.4-summary.md](./Chapter-08-OS-Primitives/8.4-summary/8.4-summary.md)"),
        ("[9.1-mutex.md](./Chapter-09-Custom-Locks/9.1-mutex.md)", "[9.1-mutex.md](./Chapter-09-Custom-Locks/9.1-mutex/9.1-mutex.md)"),
        ("[9.1.1-mutex-baseline.md](./Chapter-09-Custom-Locks/9.1.1-mutex-baseline.md)", "[9.1.1-mutex-baseline.md](./Chapter-09-Custom-Locks/9.1-mutex/9.1.1-mutex-baseline.md)"),
        ("[9.1.2-three-state-avoid-syscall.md](./Chapter-09-Custom-Locks/9.1.2-three-state-avoid-syscall.md)", "[9.1.2-three-state-avoid-syscall.md](./Chapter-09-Custom-Locks/9.1-mutex/9.1.2-three-state-avoid-syscall.md)"),
        ("[9.1.3-hybrid-lock.md](./Chapter-09-Custom-Locks/9.1.3-hybrid-lock.md)", "[9.1.3-hybrid-lock.md](./Chapter-09-Custom-Locks/9.1-mutex/9.1.3-hybrid-lock.md)"),
        ("[9.1.4-false-sharing-benchmarks.md](./Chapter-09-Custom-Locks/9.1.4-false-sharing-benchmarks.md)", "[9.1.4-false-sharing-benchmarks.md](./Chapter-09-Custom-Locks/9.1-mutex/9.1.4-false-sharing-benchmarks.md)"),
        ("[9.2-condition-variable.md](./Chapter-09-Custom-Locks/9.2-condition-variable.md)", "[9.2-condition-variable.md](./Chapter-09-Custom-Locks/9.2-condition-variable/9.2-condition-variable.md)"),
        ("[9.2.1-condvar-avoid-syscall.md](./Chapter-09-Custom-Locks/9.2.1-condvar-avoid-syscall.md)", "[9.2.1-condvar-avoid-syscall.md](./Chapter-09-Custom-Locks/9.2-condition-variable/9.2.1-condvar-avoid-syscall.md)"),
        ("[9.2.2-spurious-wakeup-notify.md](./Chapter-09-Custom-Locks/9.2.2-spurious-wakeup-notify.md)", "[9.2.2-spurious-wakeup-notify.md](./Chapter-09-Custom-Locks/9.2-condition-variable/9.2.2-spurious-wakeup-notify.md)"),
        ("[9.3-reader-writer-lock.md](./Chapter-09-Custom-Locks/9.3-reader-writer-lock.md)", "[9.3-reader-writer-lock.md](./Chapter-09-Custom-Locks/9.3-reader-writer-lock/9.3-reader-writer-lock.md)"),
        ("[9.3.1-writer-busy-loop.md](./Chapter-09-Custom-Locks/9.3.1-writer-busy-loop.md)", "[9.3.1-writer-busy-loop.md](./Chapter-09-Custom-Locks/9.3-reader-writer-lock/9.3.1-writer-busy-loop.md)"),
        ("[9.3.2-writer-starvation-state-machine.md](./Chapter-09-Custom-Locks/9.3.2-writer-starvation-state-machine.md)", "[9.3.2-writer-starvation-state-machine.md](./Chapter-09-Custom-Locks/9.3-reader-writer-lock/9.3.2-writer-starvation-state-machine.md)"),
        ("[9.3.3-vs-std-rwlock.md](./Chapter-09-Custom-Locks/9.3.3-vs-std-rwlock.md)", "[9.3.3-vs-std-rwlock.md](./Chapter-09-Custom-Locks/9.3-reader-writer-lock/9.3.3-vs-std-rwlock.md)"),
        ("[9.4-summary.md](./Chapter-09-Custom-Locks/9.4-summary.md)", "[9.4-summary.md](./Chapter-09-Custom-Locks/9.4-summary/9.4-summary.md)"),
        ("[10.1-semaphores.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.1-semaphores.md)", "[10.1-semaphores.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.1-semaphores/10.1-semaphores.md)"),
        ("[10.1.1-semaphore-basics.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.1.1-semaphore-basics.md)", "[10.1.1-semaphore-basics.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.1-semaphores/10.1.1-semaphore-basics.md)"),
        ("[10.1.2-binary-vs-counting.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.1.2-binary-vs-counting.md)", "[10.1.2-binary-vs-counting.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.1-semaphores/10.1.2-binary-vs-counting.md)"),
        ("[10.1-semaphores/](./Chapter-10-Advanced-Concurrent-Data-Structures/10.1-semaphores/)", "[10.1-semaphores/code/](./Chapter-10-Advanced-Concurrent-Data-Structures/10.1-semaphores/code/)"),
        ("[10.2-rcu.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.2-rcu.md)", "[10.2-rcu.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.2-rcu/10.2-rcu.md)"),
        ("[10.2.1-rcu-pattern.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.2.1-rcu-pattern.md)", "[10.2.1-rcu-pattern.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.2-rcu/10.2.1-rcu-pattern.md)"),
        ("[10.2.2-rcu-reclamation.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.2.2-rcu-reclamation.md)", "[10.2.2-rcu-reclamation.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.2-rcu/10.2.2-rcu-reclamation.md)"),
        ("[10.2-rcu/](./Chapter-10-Advanced-Concurrent-Data-Structures/10.2-rcu/)", "[10.2-rcu/code/](./Chapter-10-Advanced-Concurrent-Data-Structures/10.2-rcu/code/)"),
        ("[10.3-lock-free-linked-list.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.3-lock-free-linked-list.md)", "[10.3-lock-free-linked-list.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.3-lock-free-linked-list/10.3-lock-free-linked-list.md)"),
        ("[10.3.1-lock-free-stack-push.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.3.1-lock-free-stack-push.md)", "[10.3.1-lock-free-stack-push.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.3-lock-free-linked-list/10.3.1-lock-free-stack-push.md)"),
        ("[10.3.2-aba-recycling.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.3.2-aba-recycling.md)", "[10.3.2-aba-recycling.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.3-lock-free-linked-list/10.3.2-aba-recycling.md)"),
        ("[10.3-lock-free-linked-list/](./Chapter-10-Advanced-Concurrent-Data-Structures/10.3-lock-free-linked-list/)", "[10.3-lock-free-linked-list/code/](./Chapter-10-Advanced-Concurrent-Data-Structures/10.3-lock-free-linked-list/code/)"),
        ("[10.4-queue-based-locks.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.4-queue-based-locks.md)", "[10.4-queue-based-locks.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.4-queue-based-locks/10.4-queue-based-locks.md)"),
        ("[10.5-blocking-locks.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.5-blocking-locks.md)", "[10.5-blocking-locks.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.5-blocking-locks/10.5-blocking-locks.md)"),
        ("[10.6-seqlocks.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.6-seqlocks.md)", "[10.6-seqlocks.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.6-seqlocks/10.6-seqlocks.md)"),
        ("[10.7-teaching-materials.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7-teaching-materials.md)", "[10.7-teaching-materials.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7-teaching-materials/10.7-teaching-materials.md)"),
        ("[10.7.1-design-top-down.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7.1-design-top-down.md)", "[10.7.1-design-top-down.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7-teaching-materials/10.7.1-design-top-down.md)"),
        ("[10.7.2-performance-practices.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7.2-performance-practices.md)", "[10.7.2-performance-practices.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7-teaching-materials/10.7.2-performance-practices.md)"),
        ("[10.7.3-debug-pitfalls.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7.3-debug-pitfalls.md)", "[10.7.3-debug-pitfalls.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7-teaching-materials/10.7.3-debug-pitfalls.md)"),
        ("[10.7.4-interview-cheatsheet.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7.4-interview-cheatsheet.md)", "[10.7.4-interview-cheatsheet.md](./Chapter-10-Advanced-Concurrent-Data-Structures/10.7-teaching-materials/10.7.4-interview-cheatsheet.md)"),
        ("| 7.4 | [7.4-memory-fences.md](./Chapter-07-Processors/7.4-memory-fences/7.4-memory-fences.md) | 见 Ch3 `3.4-fences` |", "| 7.4 | [7.4-memory-fences.md](./Chapter-07-Processors/7.4-memory-fences/7.4-memory-fences.md) | 见 Ch3 [3.4-fences/code/](./Chapter-03-Memory-Ordering/3.4-fences/code/) |"),
    ]
    for old, new in replacements:
        t = t.replace(old, new)
    # ch6 flat md paths
    for n in ["6.1", "6.2", "6.3", "6.4", "6.5", "6.6", "6.7"]:
        slug_map = {
            "6.1": "6.1-basic-reference-counting",
            "6.2": "6.2-testing-it",
            "6.3": "6.3-mutation",
            "6.4": "6.4-weak-pointers",
            "6.5": "6.5-testing-weak",
            "6.6": "6.6-optimizing",
            "6.7": "6.7-summary",
        }
        slug = slug_map[n]
        t = t.replace(
            f"[{slug}.md](./Chapter-06-Custom-Arc/{slug}.md)",
            f"[{slug}.md](./Chapter-06-Custom-Arc/{slug}/{slug}.md)",
        )
    table.write_text(t, encoding="utf-8", newline="\n")
    print("fixed 章节与小节对照表.md")

    # Ch3 index demo path
    ch3 = ATOMIC / "Chapter-03-Memory-Ordering" / "本章学习笔记.md"
    t3 = ch3.read_text(encoding="utf-8")
    t3 = t3.replace(
        "[Ch04 4.1](../Chapter-04-Spin-Locks/4.1-minimal-implementation/)",
        "[Ch04 4.1 code](../Chapter-04-Spin-Locks/4.1-minimal-implementation/code/)",
    )
    ch3.write_text(t3, encoding="utf-8", newline="\n")

    print("intra-section link fixes done")


if __name__ == "__main__":
    main()
