#!/usr/bin/env python3
"""按模块内阅读顺序重编号 M0-M4 文件夹内的书。

A 类：文件夹名 / 路径引用（精确串，含横线）
B 类：正文书号（带书名后缀，避免误伤条款号/模块号）

dry-run 模式只打印将替换的次数和文件，不写盘。
"""
import os
import sys

ROOT = r"C:\Users\12392\Desktop\hft\06-cpp"

# (old, new, kind) —— kind: "path" 或 "text"
RULES = [
    # A 类：路径/文件夹名（长串优先，避免子串问题）
    ("09-C++17-The-Complete-Guide", "01-C++17-The-Complete-Guide", "path"),
    ("10-C++20-The-Complete-Guide", "02-C++20-The-Complete-Guide", "path"),
    ("04-Effective-Modern-C++",      "01-Effective-Modern-C++",      "path"),
    ("07-Cpp-Object-Model",          "01-Cpp-Object-Model",          "path"),
    ("08-Cpp-Concurrency",           "02-Cpp-Concurrency",           "path"),
    ("05-Effective-STL",             "03-Effective-STL",             "path"),
    ("06-STL-Source-Analysis",       "04-STL-Source-Analysis",       "path"),
    ("03-More-Effective-C++",        "02-More-Effective-C++",        "path"),
    ("02-Effective-C++",             "01-Effective-C++",             "path"),

    # B 类：正文书号（带书名，长串优先）
    ("07 深度探索 C++ 对象模型", "01 深度探索 C++ 对象模型", "text"),
    ("07 C++ 对象模型",          "01 C++ 对象模型",          "text"),
    ("07 对象模型",              "01 对象模型",              "text"),
    ("08 C++ 并发编程实战",      "02 C++ 并发编程实战",      "text"),
    ("08 C++ 并发",              "02 C++ 并发",              "text"),
    ("08 并发",                  "02 并发",                  "text"),
    ("06 STL 源码剖析",          "04 STL 源码剖析",          "text"),
    ("06 STL 源码",              "04 STL 源码",              "text"),
    ("04 Effective Modern C++",  "01 Effective Modern C++",  "text"),
    ("03 More Effective C++",    "02 More Effective C++",    "text"),
    ("05 Effective STL",         "03 Effective STL",         "text"),
    ("02 Effective C++",         "01 Effective C++",         "text"),
    ("09 C++17",                 "01 C++17",                 "text"),
    ("10 C++20",                 "02 C++20",                 "text"),
]

# 按 old 长度降序，确保长串先替换（子串安全）
RULES.sort(key=lambda r: len(r[0]), reverse=True)


def collect_md_files(root):
    out = []
    for dirpath, dirnames, filenames in os.walk(root):
        # 跳过 .workbuddy / scripts / .git
        dirnames[:] = [d for d in dirnames if d not in (".workbuddy", ".git", "scripts", "__pycache__")]
        for f in filenames:
            if f.endswith(".md"):
                out.append(os.path.join(dirpath, f))
    return out


def main():
    dry = "--apply" not in sys.argv
    mode = "DRY-RUN" if dry else "APPLY"
    print(f"=== {mode} ===")

    files = collect_md_files(ROOT)
    print(f"扫描 {len(files)} 个 .md 文件\n")

    total_path = 0
    total_text = 0
    touched = []

    for fp in files:
        with open(fp, "r", encoding="utf-8") as fh:
            content = fh.read()
        original = content
        file_path_hits = 0
        file_text_hits = 0
        for old, new, kind in RULES:
            n = content.count(old)
            if n:
                content = content.replace(old, new)
                if kind == "path":
                    file_path_hits += n
                else:
                    file_text_hits += n
        if content != original:
            touched.append((fp, file_path_hits, file_text_hits))
            total_path += file_path_hits
            total_text += file_text_hits
            if not dry:
                with open(fp, "w", encoding="utf-8") as fh:
                    fh.write(content)

    print(f"{'文件':<70} {'路径':>5} {'正文':>5}")
    print("-" * 84)
    for fp, p, t in sorted(touched, key=lambda x: -(x[1] + x[2])):
        rel = os.path.relpath(fp, ROOT)
        print(f"{rel:<70} {p:>5} {t:>5}")
    print("-" * 84)
    print(f"合计：路径替换 {total_path} 处，正文替换 {total_text} 处，涉及 {len(touched)} 个文件")
    if dry:
        print("\n这是 dry-run。确认无误后加 --apply 正式执行。")


if __name__ == "__main__":
    main()
