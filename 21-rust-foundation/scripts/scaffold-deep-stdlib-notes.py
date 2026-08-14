#!/usr/bin/env python3
"""Scaffold per-section .md note files for 03-DeepRustStdLib (skip existing non-stub files)."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "03-DeepRustStdLib"

# (section_id, filename_stem_suffix, title)
CHAPTERS: dict[str, list[tuple[str, str, str]]] = {
    "chapter02_rust_features_summary": [
        ("2.1", "generics-overview", "泛型小议"),
        ("2.1.1", "generic-functions-and-traits", "基于泛型的函数及 Trait"),
        ("2.1.2", "generic-constraint-layers", "泛型约束的层次"),
        ("2.2", "memory-safety-overview", "Rust 内存安全杂述"),
        ("2.3", "extracting-from-wrappers", "获取封装类型变量的内部变量"),
        ("2.3.1", "unwrap-with-try-operator", "使用 `?` 运算符解封装"),
        ("2.3.2", "deref-coercion", "函数调用 + 自动解引用"),
        ("2.3.3", "closures-for-inner-access", "采用闭包"),
        ("2.3.4", "getting-references", "获取引用"),
        ("2.3.5", "getting-ownership", "获取所有权"),
        ("2.4", "recap", "回顾"),
    ],
    "chapter03_memory_model": [
        ("3.1", "raw-pointers", "裸指针——不安全的根源"),
        ("3.1.1", "raw-pointer-impl", "裸指针具体实现"),
        ("3.1.2", "raw-pointer-inherent-fns", "固有模块裸指针关联函数"),
        ("3.1.3", "raw-pointer-ops", "裸指针操作"),
        ("3.1.4", "raw-pointer-extra", "裸指针番外"),
        ("3.2", "maybeuninit", "MaybeUninit<T>——未初始化变量方案"),
        ("3.2.1", "maybeuninit-definition", "MaybeUninit<T> 定义"),
        ("3.2.2", "manuallydrop-definition", "ManuallyDrop<T> 定义"),
        ("3.2.3", "maybeuninit-constructors", "MaybeUninit<T> 构造函数"),
        ("3.2.4", "maybeuninit-init-fns", "MaybeUninit<T> 初始化函数"),
        ("3.2.5", "maybeuninit-array", "MaybeUninit<T> 数组类型操作"),
        ("3.2.6", "maybeuninit-cases", "典型案例"),
        ("3.3", "raw-pointers-revisited", "裸指针再论"),
        ("3.4", "nonnull", "非空裸指针——NonNull<T>"),
        ("3.4.1", "nonnull-constructors", "构造关联函数"),
        ("3.4.2", "nonnull-conversions", "类型转换函数"),
        ("3.4.3", "nonnull-other-fns", "其他函数"),
        ("3.5", "unique", "智能指针的基座——Unique<T>"),
        ("3.6", "mem-module", "mem 模块函数"),
        ("3.6.1", "mem-construct", "构造泛型变量函数"),
        ("3.6.2", "mem-ownership-transfer", "泛型变量所有权转移函数"),
        ("3.6.3", "mem-other-fns", "其他函数"),
        ("3.7", "heap-alloc", "动态内存申请及释放"),
        ("3.7.1", "memory-layout", "内存布局"),
        ("3.7.2", "alloc-free-api", "动态内存申请与释放接口"),
        ("3.8", "static-memory", "全局变量内存探讨"),
        ("3.9", "drop-summary", "drop 总结"),
        ("3.10", "ownership-lifetimes-borrow", "Rust 所有权、生命周期、借用探讨"),
        ("3.11", "recap", "回顾"),
    ],
    "chapter04_primitive_types": [
        ("4.1", "inherent-fns", "固有函数库"),
        ("4.1.1", "atomic-fns", "原子操作函数"),
        ("4.1.2", "math-bit-fns", "数学函数及位操作函数"),
        ("4.1.3", "prefetch-assert-stack-fns", "指令预取优化函数、断言类函数及栈获取函数"),
        ("4.2", "basic-types", "基本类型分析"),
        ("4.2.1", "integer-types", "整数类型"),
        ("4.2.2", "float-types", "浮点类型"),
        ("4.2.3", "option-type", "Option<T> 类型"),
        ("4.2.4", "ref-match-syntax", "引用类型 match 语法研究"),
        ("4.2.5", "result-type", "Result<T, E> 类型"),
        ("4.3", "basic-traits", "基本 Trait"),
        ("4.3.1", "marker-traits", "编译器内置 Marker Trait"),
        ("4.3.2", "arithmetic-traits", "算术运算符 Trait"),
        ("4.3.3", "try-trait", "`?` 运算符 Trait"),
        ("4.3.4", "range-traits", "范围运算符 Trait"),
        ("4.3.5", "index-traits", "索引运算符 Trait"),
    ],
    "chapter05_iterators": [
        ("5.1", "three-iterators", "三种迭代器"),
        ("5.2", "iterator-trait", "Iterator Trait 分析"),
        ("5.3", "iterator-collections", "Iterator 与其他集合类型转换"),
        ("5.4", "range-iterators", "范围类型迭代器"),
        ("5.5", "slice-iterators", "切片类型迭代器"),
        ("5.6", "string-iterators", "字符串类型迭代器"),
        ("5.7", "array-iterators", "数组类型迭代器"),
        ("5.7.1", "array-by-value-iter", "成员本身迭代器"),
        ("5.7.2", "array-by-ref-iter", "成员引用迭代器"),
        ("5.8", "iterator-adapters", "Iterator 适配器"),
        ("5.8.1", "map-adapter", "Map 适配器"),
        ("5.8.2", "chain-adapter", "Chain 适配器"),
        ("5.8.3", "other-adapters", "其他适配器"),
        ("5.9", "option-adapters", "Option<T> 适配器"),
    ],
    "chapter06_basic_types_continued": [
        ("6.1", "integer-types", "整数类型"),
        ("6.2", "bool-type", "布尔类型"),
        ("6.3", "char-type", "字符类型"),
        ("6.4", "string-type", "字符串类型"),
        ("6.5", "slice-type", "切片类型"),
    ],
    "chapter07_interior_mutability": [
        ("7.1", "borrow-borrowmut", "Borrow / BorrowMut 分析"),
        ("7.2", "cell-overview", "Cell<T> 类型分析"),
        ("7.2.1", "unsafecell", "UnsafeCell<T> 分析"),
        ("7.2.2", "cell", "Cell<T> 分析"),
        ("7.3", "refcell-overview", "RefCell<T> 类型分析"),
        ("7.3.1", "refcell-borrow-trait", "Borrow Trait 分析"),
        ("7.3.2", "refcell-borrowmut-trait", "BorrowMut Trait 分析"),
        ("7.3.3", "refcell-other-fns", "RefCell<T> 的其他函数"),
        ("7.4", "pin-unpin", "Pin<T> / Unpin<T> 类型分析"),
        ("7.5", "lazy", "Lazy<T> 类型分析"),
    ],
    "chapter08_smart_pointers": [
        ("8.1", "box", "Box<T> 类型分析"),
        ("8.2", "rawvec", "RawVec<T> 类型分析"),
        ("8.3", "vec-overview", "Vec<T> 类型分析"),
        ("8.3.1", "vec-basics", "Vec<T> 基础分析"),
        ("8.3.2", "vec-iterator", "Vec<T> 的 Iterator Trait"),
        ("8.4", "rc-overview", "Rc<T> 类型分析"),
        ("8.4.1", "rc-ctor-dtor", "Rc<T> 类型的构造函数及析构函数"),
        ("8.4.2", "rc-weak", "Weak<T> 类型分析"),
        ("8.4.3", "rc-other-fns", "Rc<T> 的其他函数"),
        ("8.5", "arc-overview", "Arc<T> 类型分析"),
        ("8.5.1", "arc-ctor-dtor", "Arc<T> 类型的构造函数及析构函数"),
        ("8.5.2", "arc-weak", "Weak<T> 类型分析"),
        ("8.5.3", "arc-other-fns", "Arc<T> 的其他函数"),
        ("8.6", "cow-overview", "Cow<'a, T> 类型分析"),
        ("8.6.1", "toowned-trait", "ToOwned Trait 分析"),
        ("8.6.2", "cow", "Cow<'a, T> 分析"),
        ("8.7", "linkedlist", "LinkedList<T> 类型分析"),
        ("8.8", "string-overview", "String 类型分析"),
        ("8.8.1", "string-basics", "初识 String 类型分析"),
        ("8.8.2", "format-string", "格式化字符串分析"),
    ],
    "chapter09_userspace_std_basics": [
        ("9.1", "rust-c-interop", "Rust 与 C 语言交互"),
        ("9.1.1", "c-type-adaptation", "C 语言的类型适配"),
        ("9.1.2", "c-va-list", "C 语言的 va_list 类型适配"),
        ("9.1.3", "c-strings", "C 语言字符串类型适配"),
        ("9.1.4", "osstring", "OsString 代码分析"),
        ("9.2", "engineering-trick", "代码工程中的一个技巧"),
        ("9.3", "std-memory-mgmt", "内存管理之 STD 库"),
        ("9.4", "syscall-wrapper", "系统调用（SYSCALL）的封装"),
        ("9.5", "fd-and-handles", "文件描述符及句柄"),
        ("9.5.1", "fd-ownership", "文件描述符所有权设计"),
        ("9.5.2", "fd-logic-layer", "文件逻辑操作适配层"),
        ("9.6", "process-overview", "进程管理"),
    ],
    "chapter10_process_management": [
        ("10.1", "anonymous-pipe", "匿名管道"),
        ("10.2", "redirection", "重定向实现分析"),
        ("10.3", "process-mgmt", "进程管理"),
        ("10.3.1", "process-os-layer", "OS 相关适配层"),
        ("10.3.2", "process-public-api", "对外接口层"),
    ],
    "chapter11_concurrency": [
        ("11.1", "futex", "Futex 分析"),
        ("11.2", "mutex-overview", "Mutex<T> 类型分析"),
        ("11.2.1", "mutex-os-layer", "OS 相关适配层"),
        ("11.2.2", "mutex-os-agnostic", "OS 无关适配层"),
        ("11.2.3", "mutex-public-api", "对外接口层"),
        ("11.3", "condvar-overview", "Condvar 类型分析"),
        ("11.3.1", "condvar-os-layer", "OS 相关适配层"),
        ("11.3.2", "condvar-os-agnostic", "OS 无关适配层"),
        ("11.3.3", "condvar-public-api", "对外接口层"),
        ("11.4", "rwlock-overview", "RwLock<T> 类型分析"),
        ("11.4.1", "rwlock-os-layer", "OS 相关适配层"),
        ("11.4.2", "rwlock-os-agnostic", "OS 无关适配层"),
        ("11.4.3", "rwlock-public-api", "对外接口层"),
        ("11.5", "barrier", "Barrier 类型分析"),
        ("11.6", "once", "Once 类型分析"),
        ("11.7", "oncelock", "OnceLock<T> 类型分析"),
        ("11.8", "lazylock", "LazyLock<T> 类型分析"),
        ("11.9", "thread-overview", "线程分析"),
        ("11.9.1", "thread-os-layer", "OS 相关适配层"),
        ("11.9.2", "thread-os-agnostic", "OS 无关适配层"),
        ("11.9.3", "thread-public-api", "对外接口层"),
        ("11.10", "mpsc-overview", "线程消息通信——MPSC"),
        ("11.10.1", "mpsc-queue", "消息队列类型——Queue<T>"),
        ("11.10.2", "mpsc-block-wake", "阻塞及唤醒信号机制"),
        ("11.10.3", "mpsc-oneshot", "一次性通信通道机制"),
        ("11.10.4", "mpsc-shared", "Shared 类型通道"),
        ("11.10.5", "mpsc-public-api", "对外接口层"),
        ("11.11", "runtime", "Rust 的 RUNTIME"),
    ],
    "chapter12_filesystem": [
        ("12.1", "fs-os-layer", "OS 相关适配层"),
        ("12.1.1", "path-types", "路径名类型分析"),
        ("12.1.2", "file-ops", "普通文件操作分析"),
        ("12.1.3", "dir-ops", "目录操作分析"),
        ("12.2", "fs-public-api", "对外接口层"),
    ],
    "chapter13_io": [
        ("13.1", "stdin-overview", "标准输入 Stdin 类型分析"),
        ("13.1.1", "read-trait", "Read Trait"),
        ("13.1.2", "vec-read-write", "向量读/写类型分析"),
        ("13.1.3", "stdin-public-api", "对外接口层"),
        ("13.2", "stdout", "标准输出 Stdout 类型分析"),
        ("13.3", "network-io", "网络 I/O"),
    ],
    "chapter14_async": [
        ("14.1", "coroutine-framework", "Rust 协程框架简析"),
        ("14.1.1", "coroutine-overview", "协程概述"),
        ("14.1.2", "io-multiplexing", "Rust 的 I/O 多路复用"),
        ("14.2", "coroutine-types", "Rust 协程支持类型简析"),
        ("14.2.1", "coroutine-management", "Rust 协程管理"),
        ("14.2.2", "future-trait", "Future Trait 分析"),
    ],
}

CHAPTER_NUM: dict[str, int] = {
    "chapter02_rust_features_summary": 2,
    "chapter03_memory_model": 3,
    "chapter04_primitive_types": 4,
    "chapter05_iterators": 5,
    "chapter06_basic_types_continued": 6,
    "chapter07_interior_mutability": 7,
    "chapter08_smart_pointers": 8,
    "chapter09_userspace_std_basics": 9,
    "chapter10_process_management": 10,
    "chapter11_concurrency": 11,
    "chapter12_filesystem": 12,
    "chapter13_io": 13,
    "chapter14_async": 14,
}

STUB_MARKERS = ("（待补充）", "## 我的笔记", "阅读原书后填写")


def filename(section_id: str, suffix: str) -> str:
    return f"{section_id}-{suffix}.md"


def nav_line(chapter_num: int, prev_f: str | None, next_f: str | None) -> str:
    parts = [f"> 章索引：[第 {chapter_num} 章](./README.md)"]
    if prev_f:
        parts.append(f"前：[{prev_f.replace('.md', '')}](./{prev_f})")
    if next_f:
        parts.append(f"后：[{next_f.replace('.md', '')}](./{next_f})")
    return " · ".join(parts)


def is_substantive(path: Path) -> bool:
    if not path.exists():
        return False
    text = path.read_text(encoding="utf-8")
    if len(text) > 800 and "（待补充）" not in text:
        return True
    return False


def stub_content(
    section_id: str, title: str, chapter_num: int, prev_f: str | None, next_f: str | None
) -> str:
    nav = nav_line(chapter_num, prev_f, next_f)
    return f"""# {section_id} {title}

{nav}

---

## 一句话

（待补充 — 用一句话说清本节在 `libstd` / 语言里的位置）

---

## 原书要点

（阅读原书后填写：定义、规则、易错点）

---

## 源码锚点

| 位置 | 说明 |
|------|------|
| `library/` | （待补充：crate 路径、`mod` 入口） |

---

## 我的笔记

（刷书、对照源码、刷题、项目实战后在此迭代）

---

## 相关

- 
"""


def update_readme(chapter_dir: Path, entries: list[tuple[str, str, str]]) -> None:
    readme = chapter_dir / "README.md"
    if not readme.exists():
        return
    text = readme.read_text(encoding="utf-8")
    # Build new index table rows
    rows = []
    for sid, suffix, title in entries:
        fn = filename(sid, suffix)
        rows.append(f"| **{sid}** | {title} | [笔记](./{fn}) |")
    index_block = "\n".join(rows)

    marker_start = "<!-- AUTO:SECTION-INDEX -->"
    marker_end = "<!-- /AUTO:SECTION-INDEX -->"
    new_section = f"""{marker_start}

| 节 | 主题 | 笔记 |
|:---:|------|------|
{index_block}

{marker_end}"""

    if marker_start in text:
        before = text.split(marker_start)[0]
        after = text.split(marker_end)[1] if marker_end in text else ""
        text = before + new_section + after
    else:
        # Insert after first --- following 阅读顺序
        insert_at = text.find("\n---\n", text.find("阅读顺序"))
        if insert_at == -1:
            text = text.rstrip() + "\n\n" + new_section + "\n"
        else:
            text = text[: insert_at + 5] + "\n\n" + new_section + text[insert_at + 5 :]
    readme.write_text(text, encoding="utf-8")


def main() -> None:
    created = 0
    skipped = 0
    for chapter, entries in CHAPTERS.items():
        chapter_dir = ROOT / chapter
        chapter_num = CHAPTER_NUM[chapter]
        chapter_dir.mkdir(parents=True, exist_ok=True)
        files = [filename(sid, suf) for sid, suf, _ in entries]
        for i, (sid, suffix, title) in enumerate(entries):
            path = chapter_dir / filename(sid, suffix)
            prev_f = files[i - 1] if i > 0 else None
            next_f = files[i + 1] if i < len(files) - 1 else None
            if is_substantive(path):
                skipped += 1
                continue
            content = stub_content(sid, title, chapter_num, prev_f, next_f)
            if not path.exists():
                created += 1
            path.write_text(content, encoding="utf-8")
        update_readme(chapter_dir, entries)
    print(f"Done: created/updated stubs, skipped substantive={skipped}")


if __name__ == "__main__":
    main()
