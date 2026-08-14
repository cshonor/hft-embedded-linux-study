# -*- coding: utf-8 -*-
"""Create X.Y-slug/ + *-demo.rs for every section listed in 本章学习笔记.md."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Chapters that mostly use Tokio in demos
TOKIO_CHAPTERS = {
    "ch01_async_intro",
    "ch02_async_rust_core",
    "ch03_custom_task_queue",
    "ch06_reactive_async_streams",
    "ch07_tokio_graceful_shutdown",
    "ch08_actor_model",
    "ch09_async_design_patterns",
    "ch11_async_testing_debugging",
}

SUMMARY_RE = re.compile(r"summary|总结", re.I)


def parse_index(ch_dir: Path) -> list[tuple[str, str]]:
    idx = ch_dir / "本章学习笔记.md"
    if not idx.exists():
        return []
    text = idx.read_text(encoding="utf-8")
    rows = re.findall(
        r"\| (\d+\.\d+) \|.*?\[([^\]]+\.md)\]\([^)]+\).*\| ([^|]+) \|",
        text,
    )
    out = []
    for _sec, md, demo_col in rows:
        slug = md.replace(".md", "")
        out.append((slug, demo_col.strip()))
    return out


def is_summary(slug: str) -> bool:
    return bool(SUMMARY_RE.search(slug))


def tokio_template(slug: str, title: str) -> str:
    return f"""//! §{title} — 教学 demo（Tokio）。
use tokio::time::{{sleep, Duration}};

#[tokio::main]
async fn main() {{
    println!("{{}} — 见同章对应 .md 精读", "{slug}");
    sleep(Duration::from_millis(50)).await;
    println!("done");
}}
"""


def std_template(slug: str, title: str) -> str:
    crate = slug.replace(".", "_").replace("-", "_")
    return f"""//! §{title} — 教学 demo（std）。
#![crate_name = "demo_{crate}"]

fn main() {{
    println!("{{}} — 见同章对应 .md 精读", "{slug}");
}}
"""


def summary_template(ch_dir: Path, slug: str, title: str) -> str:
    siblings = []
    for d in sorted(ch_dir.iterdir()):
        if d.is_dir() and re.match(r"\d+\.\d+-", d.name) and d.name != slug:
            demos = list(d.glob("*-demo.rs"))
            if demos:
                siblings.append(d.name)
    sibs = ", ".join(siblings[:8])
    if len(siblings) > 8:
        sibs += ", …"
    return f"""//! §{title} — 本章小结（指向同章其它 demo）。
#![crate_name = "demo_{slug.replace('.', '_').replace('-', '_')}"]

fn main() {{
    println!("{{}} Summary", "{slug}");
    println!("本章其它 demo 目录: {sibs}");
}}
"""


def main() -> None:
    created = 0
    for ch_dir in sorted(ROOT.glob("ch*")):
        if not ch_dir.is_dir():
            continue
        ch_name = ch_dir.name
        use_tokio = ch_name in TOKIO_CHAPTERS
        for slug, demo_col in parse_index(ch_dir):
            sec = slug.split("-", 1)[0]  # e.g. 11.2
            section_dir = ch_dir / slug
            if any(section_dir.glob("*-demo.rs")):
                continue
            demo_path = section_dir / f"{slug}-demo.rs"
            section_dir.mkdir(parents=True, exist_ok=True)
            if is_summary(slug):
                content = summary_template(ch_dir, slug, sec)
            elif use_tokio and ch_name != "ch10_dependency_free_async_server":
                content = tokio_template(slug, sec)
            else:
                content = std_template(slug, sec)
            demo_path.write_text(content, encoding="utf-8")
            print("created:", demo_path.relative_to(ROOT))
            created += 1
    print(f"Done. Created {created} demo(s).")


if __name__ == "__main__":
    main()
