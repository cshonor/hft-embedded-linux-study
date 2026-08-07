#!/usr/bin/env python3
"""07↔08 笔记级双向交叉引用脚本
在 LKD (07) 和 ULK (08) 对应章节之间建立笔记级别的双向链接。

07 笔记结尾: ...--- (无导航行)
08 笔记结尾: ...---\n\n← [prev] · [next] (有导航行)

链接格式:
  07 侧: 在最后 --- 前插入 > ↔ [ULK ChX §Y name](path)
  08 侧: 在 ← 导航行后追加 > ↔ [LKD ChX §Y name](path)
"""
import os

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BASE_07 = os.path.join(REPO_ROOT, "07-linux-kernel", "00_Book_3rd_Notes")
BASE_08 = os.path.join(REPO_ROOT, "08-linux-kernel-deep")

# 07 文件相对 BASE_07 的路径 → 08 文件相对 BASE_08 的路径 + 描述
# 格式: (07_rel, [(08_rel, 08_desc), ...])
# 一篇 07 可以对应多篇 08，反之亦然

CROSS_REFS = [
    # === 调度: 07 Ch4 ↔ 08 Ch7 ===
    ("chapter-04-process-scheduling/notes/section-4.1-多任务与调度器演进.md",
     "chapter-07-process-scheduling/notes/section-1-本章定位.md",
     "ULK Ch7 §1 本章定位"),
    ("chapter-04-process-scheduling/notes/section-4.2-调度策略.md",
     "chapter-07-process-scheduling/notes/section-2-调度策略与抢占.md",
     "ULK Ch7 §2 调度策略与抢占"),
    ("chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md",
     "chapter-07-process-scheduling/notes/section-4-调度算法与核心函数.md",
     "ULK Ch7 §4 调度算法与核心函数"),
    ("chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md",
     "chapter-07-process-scheduling/notes/section-2-调度策略与抢占.md",
     "ULK Ch7 §2 调度策略与抢占"),
    ("chapter-04-process-scheduling/notes/section-4.6-实时调度策略.md",
     "chapter-07-process-scheduling/notes/section-2-调度策略与抢占.md",
     "ULK Ch7 §2 调度策略与抢占"),
    ("chapter-04-process-scheduling/notes/section-4.7-与调度相关的系统调用.md",
     "chapter-07-process-scheduling/notes/section-6-调度相关系统调用.md",
     "ULK Ch7 §6 调度相关系统调用"),

    # === 中断+下半部: 07 Ch7+Ch8 ↔ 08 Ch4 ===
    ("chapter-07-interrupts/notes/section-7.1-中断的概念.md",
     "chapter-04-interrupts-and-exceptions/notes/section-2-中断与异常分类.md",
     "ULK Ch4 §2 中断与异常分类"),
    ("chapter-07-interrupts/notes/section-7.2-中断处理程序.md",
     "chapter-04-interrupts-and-exceptions/notes/section-6-IO中断处理.md",
     "ULK Ch4 §6 IO中断处理"),
    ("chapter-07-interrupts/notes/section-7.3-上半部与下半部.md",
     "chapter-04-interrupts-and-exceptions/notes/section-7-可延迟函数与工作队列.md",
     "ULK Ch4 §7 可延迟函数与工作队列"),
    ("chapter-07-interrupts/notes/section-7.6-中断处理机制的实现.md",
     "chapter-04-interrupts-and-exceptions/notes/section-3-IDT与门描述符.md",
     "ULK Ch4 §3 IDT与门描述符"),
    ("chapter-08-bottom-halves/notes/section-8.3-软中断.md",
     "chapter-04-interrupts-and-exceptions/notes/section-7-可延迟函数与工作队列.md",
     "ULK Ch4 §7 可延迟函数与工作队列"),
    ("chapter-08-bottom-halves/notes/section-8.4-tasklet.md",
     "chapter-04-interrupts-and-exceptions/notes/section-7-可延迟函数与工作队列.md",
     "ULK Ch4 §7 可延迟函数与工作队列"),
    ("chapter-08-bottom-halves/notes/section-8.5-工作队列.md",
     "chapter-04-interrupts-and-exceptions/notes/section-7-可延迟函数与工作队列.md",
     "ULK Ch4 §7 可延迟函数与工作队列"),

    # === 同步: 07 Ch9+Ch10 ↔ 08 Ch5 ===
    ("chapter-09-kernel-sync-intro/notes/section-9.1-临界区与竞态条件.md",
     "chapter-05-kernel-synchronization/notes/section-1-本章定位.md",
     "ULK Ch5 §1 本章定位"),
    ("chapter-10-sync-methods/notes/section-10.2-自旋锁.md",
     "chapter-05-kernel-synchronization/notes/section-4-自旋锁.md",
     "ULK Ch5 §4 自旋锁"),
    ("chapter-10-sync-methods/notes/section-10.4-信号量.md",
     "chapter-05-kernel-synchronization/notes/section-6-信号量与完成变量.md",
     "ULK Ch5 §6 信号量与完成变量"),
    ("chapter-10-sync-methods/notes/section-10.5-互斥体.md",
     "chapter-05-kernel-synchronization/notes/section-6-信号量与完成变量.md",
     "ULK Ch5 §6 信号量与完成变量"),
    ("chapter-10-sync-methods/notes/section-10.8-顺序锁.md",
     "chapter-05-kernel-synchronization/notes/section-5-顺序锁与RCU.md",
     "ULK Ch5 §5 顺序锁与RCU"),
    ("chapter-10-sync-methods/notes/section-10.11-选型速查Ch-9--Ch-10.md",
     "chapter-05-kernel-synchronization/notes/section-7-选型与实例.md",
     "ULK Ch5 §7 选型与实例"),

    # === 定时器: 07 Ch11 ↔ 08 Ch6 ===
    ("chapter-11-timers/notes/section-11.1-内核时间概念与节拍率.md",
     "chapter-06-timing/notes/section-1-本章定位.md",
     "ULK Ch6 §1 本章定位"),
    ("chapter-11-timers/notes/section-11.3-硬件时钟和定时器.md",
     "chapter-06-timing/notes/section-2-硬件时钟与定时器.md",
     "ULK Ch6 §2 硬件时钟与定时器"),
    ("chapter-11-timers/notes/section-11.4-定时器中断处理程序.md",
     "chapter-06-timing/notes/section-4-更新时间与统计.md",
     "ULK Ch6 §4 更新时间与统计"),
    ("chapter-11-timers/notes/section-11.6-动态定时器.md",
     "chapter-06-timing/notes/section-5-软件定时器与延迟函数.md",
     "ULK Ch6 §5 软件定时器与延迟函数"),

    # === 内存管理: 07 Ch12 ↔ 08 Ch8 ===
    ("chapter-12-memory-management/notes/section-12.2-页.md",
     "chapter-08-memory-management/notes/section-2-页框管理.md",
     "ULK Ch8 §2 页框管理"),
    ("chapter-12-memory-management/notes/section-12.5-kmalloc-与-kfree.md",
     "chapter-08-memory-management/notes/section-3-Slab分配器.md",
     "ULK Ch8 §3 Slab分配器"),
    ("chapter-12-memory-management/notes/section-12.6-vmalloc.md",
     "chapter-08-memory-management/notes/section-4-非连续内存与vmalloc.md",
     "ULK Ch8 §4 非连续内存与vmalloc"),
    ("chapter-12-memory-management/notes/section-12.7-Slab-层.md",
     "chapter-08-memory-management/notes/section-3-Slab分配器.md",
     "ULK Ch8 §3 Slab分配器"),

    # === 地址空间: 07 Ch15 ↔ 08 Ch9 ===
    ("chapter-15-process-address-space/notes/section-15.2-内存描述符.md",
     "chapter-09-process-address-space/notes/section-2-内存描述符.md",
     "ULK Ch9 §2 内存描述符"),
    ("chapter-15-process-address-space/notes/section-15.3-虚拟内存区域.md",
     "chapter-09-process-address-space/notes/section-3-内存区VMA.md",
     "ULK Ch9 §3 内存区VMA"),
    ("chapter-15-process-address-space/notes/section-15.7-页表.md",
     "chapter-09-process-address-space/notes/section-4-缺页异常.md",
     "ULK Ch9 §4 缺页异常"),
    ("chapter-15-process-address-space/notes/section-15.8-从访问到缺页概念.md",
     "chapter-09-process-address-space/notes/section-5-请求调页.md",
     "ULK Ch9 §5 请求调页"),

    # === 系统调用: 07 Ch5 ↔ 08 Ch10 ===
    ("chapter-05-system-calls/notes/section-5.2-系统调用基础.md",
     "chapter-10-system-calls/notes/section-2-POSIX-API与系统调用.md",
     "ULK Ch10 §2 POSIX-API与系统调用"),
    ("chapter-05-system-calls/notes/section-5.3-系统调用处理程序.md",
     "chapter-10-system-calls/notes/section-3-分派表与服务例程.md",
     "ULK Ch10 §3 分派表与服务例程"),
    ("chapter-05-system-calls/notes/section-5.4-实现与参数验证.md",
     "chapter-10-system-calls/notes/section-6-参数验证与内核封装.md",
     "ULK Ch10 §6 参数验证与内核封装"),
]


def compute_rel_path(from_file, to_file):
    """计算从 from_file 到 to_file 的相对路径"""
    from_dir = os.path.dirname(from_file)
    rel = os.path.relpath(to_file, from_dir)
    return rel.replace(os.sep, "/")


def insert_link_07(content, link_line):
    """07 笔记结尾是 --- (无导航行)，在最后一个 --- 前插入链接行"""
    stripped = content.rstrip()
    lines = stripped.split("\n")
    # 从后向前找最后一个 --- 行
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].strip() == "---":
            lines.insert(i, link_line)
            lines.insert(i, "")
            return "\n".join(lines) + "\n"
    # 如果没找到 ---，追加到末尾
    lines.append("")
    lines.append(link_line)
    return "\n".join(lines) + "\n"


def insert_link_08(content, link_line):
    """08 笔记结尾是 ---\\n\\n← [prev] · [next]，在 ← 导航行后追加链接行"""
    stripped = content.rstrip()
    lines = stripped.split("\n")
    # 从后向前找 ← 开头的行
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].startswith("←"):
            lines.insert(i + 1, link_line)
            return "\n".join(lines) + "\n"
    # 如果没找到 ← 行，在最后一个 --- 前插入
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].strip() == "---":
            lines.insert(i, link_line)
            lines.insert(i, "")
            return "\n".join(lines) + "\n"
    lines.append("")
    lines.append(link_line)
    return "\n".join(lines) + "\n"


# === 主流程 ===
ok_07 = 0
ok_08 = 0
skip = 0
miss = 0

for ref_07, ref_08, desc_08 in CROSS_REFS:
    path_07 = os.path.join(BASE_07, ref_07)
    path_08 = os.path.join(BASE_08, ref_08)

    if not os.path.exists(path_07) or not os.path.exists(path_08):
        missing = ref_07 if not os.path.exists(path_07) else ref_08
        print(f"MISSING: {missing}")
        miss += 1
        continue

    # 07 侧 → 指向 08
    with open(path_07, "r", encoding="utf-8") as f:
        content_07 = f.read()
    link_to_08 = f"> ↔ [{desc_08}]({compute_rel_path(path_07, path_08)})"
    if "↔" in content_07 and desc_08 in content_07:
        print(f"SKIP 07: {ref_07}")
        skip += 1
    else:
        content_07 = insert_link_07(content_07, link_to_08)
        with open(path_07, "w", encoding="utf-8") as f:
            f.write(content_07)
        ok_07 += 1
        print(f"OK 07→08: {ref_07}")

    # 08 侧 → 指向 07
    with open(path_08, "r", encoding="utf-8") as f:
        content_08 = f.read()
    # 从 08 文件名提取章节号和标题
    import re
    m = re.search(r"section-(\d+\.\d+)-(.+)\.md", ref_07)
    if m:
        sec_num = m.group(1)
        sec_title = m.group(2)
        # 从 chapter 名提取章号
        m2 = re.search(r"chapter-(\d+)-", ref_07)
        ch_num = m2.group(1) if m2 else "?"
        desc_07 = f"LKD Ch{ch_num} §{sec_num} {sec_title}"
    else:
        desc_07 = f"LKD {ref_07}"
    link_to_07 = f"> ↔ [{desc_07}]({compute_rel_path(path_08, path_07)})"
    if "↔" in content_08 and desc_07 in content_08:
        print(f"SKIP 08: {ref_08}")
    else:
        content_08 = insert_link_08(content_08, link_to_07)
        with open(path_08, "w", encoding="utf-8") as f:
            f.write(content_08)
        ok_08 += 1
        print(f"OK 08→07: {ref_08}")

print(f"\n=== Done: {ok_07} links added to 07, {ok_08} links added to 08, {skip} SKIP, {miss} MISSING ===")
