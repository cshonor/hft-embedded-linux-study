#!/usr/bin/env python3
"""Scaffold 09-C++17-The-Complete-Guide chapter directories (Josuttis TOC)."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "09-C++17-The-Complete-Guide"

CHAPTERS = [
    ("ch01-structured-bindings", "第 1 章 结构化绑定", "Structured Bindings"),
    ("ch02-if-switch-init", "第 2 章 if/switch 带初始化", "if and switch with Initialization"),
    ("ch03-inline-variables", "第 3 章 内联变量", "Inline Variables"),
    ("ch04-aggregate-extensions", "第 4 章 聚合类型扩展", "Aggregate Extensions"),
    ("ch05-copy-elision", "第 5 章 强制拷贝省略与未物化传值", "Mandatory Copy Elision or Passing Unmaterialized Objects"),
    ("ch06-lambda-extensions", "第 6 章 Lambda 扩展", "Lambda Extensions"),
    ("ch07-attributes", "第 7 章 新属性与属性扩展", "New Attributes and Attribute Features"),
    ("ch08-other-language-features", "第 8 章 其他语言特性", "Other Language Features"),
    ("ch09-ctad", "第 9 章 类模板参数推导 CTAD", "Class Template Argument Deduction"),
    ("ch10-constexpr-if", "第 10 章 编译期 if", "Compile-Time if"),
    ("ch11-fold-expressions", "第 11 章 折叠表达式", "Fold Expressions"),
    ("ch12-string-literals-nttp", "第 12 章 字符串字面量作模板参数", "Dealing with String Literals as Template Parameters"),
    ("ch13-auto-template-params", "第 13 章 auto 作模板参数", "Placeholder Types like auto as Template Parameters"),
    ("ch14-extended-using", "第 14 章 扩展 using 声明", "Extended Using Declarations"),
    ("ch15-optional", "第 15 章 std::optional", "std::optional<>"),
    ("ch16-variant", "第 16 章 std::variant", "std::variant<>"),
    ("ch17-any", "第 17 章 std::any", "std::any"),
    ("ch18-byte", "第 18 章 std::byte", "std::byte"),
    ("ch19-string-view", "第 19 章 字符串视图", "String Views"),
    ("ch20-filesystem", "第 20 章 文件系统库", "The Filesystem Library"),
    ("ch21-type-traits", "第 21 章 type_traits 扩展", "Extensions of Type Traits"),
    ("ch22-parallel-stl", "第 22 章 并行 STL 算法", "Parallel STL Algorithms"),
    ("ch23-new-stl-algorithms", "第 23 章 新 STL 算法详解", "New STL Algorithms in Detail"),
    ("ch24-subsequence-searchers", "第 24 章 子串与子序列搜索器", "Substring and Subsequence Searchers"),
    ("ch25-utility-algorithms", "第 25 章 其他工具函数与算法", "Other Utility Functions and Algorithms"),
    ("ch26-container-extensions", "第 26 章 容器与 string 扩展", "Container and String Extensions"),
    ("ch27-multithreading", "第 27 章 多线程与并发库", "Multi-Threading and Concurrency"),
    ("ch28-small-library-features", "第 28 章 其他标准库小改进", "Other Small Library Features and Modifications"),
    ("ch29-pmr", "第 29 章 多态内存资源 PMR", "Polymorphic Memory Resources (PMR)"),
    ("ch30-over-aligned-new", "第 30 章 超对齐 new/delete", "new and delete with Over-Aligned Data"),
    ("ch31-to-from-chars", "第 31 章 to_chars / from_chars", "std::to_chars() and std::from_chars()"),
    ("ch32-launder", "第 32 章 std::launder", "std::launder()"),
    ("ch33-generic-code-improvements", "第 33 章 泛型代码实现改进", "Improvements for Implementing Generic Code"),
    ("ch34-cpp17-settings", "第 34 章 常用 C++17 编译设置", "Common C++17 Settings"),
    ("ch35-deprecated-removed", "第 35 章 弃用与移除特性", "Deprecated and Removed Features"),
]

HFT_HIGHLIGHTS = {
    "ch01-structured-bindings",
    "ch11-fold-expressions",
    "ch19-string-view",
    "ch22-parallel-stl",
    "ch29-pmr",
    "ch31-to-from-chars",
}


def chapter_readme(slug: str, cn: str, en: str) -> str:
    hft = ""
    if slug in HFT_HIGHLIGHTS:
        hft = "\n\n## HFT 关联\n\n> 待补充：低延迟 / 行情解析 / 并行批处理等场景。\n"
    return f"""# {cn}

**{en}**

## 笔记

> 待补充
{hft}"""


def main() -> None:
    ROOT.mkdir(exist_ok=True)

    (ROOT / "preface").mkdir(exist_ok=True)
    (ROOT / "preface" / "README.md").write_text(
        """# 前言 Preface

## 笔记

> 待补充
""",
        encoding="utf-8",
    )

    rows = []
    for slug, cn, en in CHAPTERS:
        d = ROOT / slug
        d.mkdir(exist_ok=True)
        (d / "README.md").write_text(chapter_readme(slug, cn, en), encoding="utf-8")
        rows.append(f"| {slug.split('-')[0][2:]} | [{cn}](./{slug}/) | {en} |")

    progress = "\n".join(f"- [ ] 第 {i} 章" for i in range(1, 36))

    readme = f"""# 《C++17 - The Complete Guide》章节索引

> Nicolai Josuttis 著，与 [10-C++20-The-Complete-Guide](../10-C++20-The-Complete-Guide/) 同作者。**08 并发 → 09 C++17 → 10 C++20** 按版本递进，C++17 是 HFT 技术栈里大量「现代特性」的过渡基线。

## 为什么插在 08 与 09 之间

| C++17 特性 | HFT / 低延迟场景 |
|------------|------------------|
| 结构化绑定 | 解包 tick / order 字段，少写临时变量 |
| 折叠表达式 | 可变参模板、编译期聚合日志/校验 |
| `if constexpr` | 热路径零开销分支，替代 SFINAE |
| `string_view` | 零拷贝解析 FIX/CSV 字段 |
| 并行 STL | 离线回测、批处理行情（见 [08 ch10](../08-Cpp-Concurrency/ch10-parallel-algorithms/)） |
| `optional` / `variant` | 可空字段、多态消息体而不必继承 |
| PMR / `to_chars` | 可控分配、无 iostream 的快速数值格式化 |

许多特性在 **C++20 中继续完善**（如 Concepts 约束 CTAD、`jthread` 扩展并发模型、Ranges 延续 `string_view` 思路），先吃透 17 再读 20 更顺。

## 全书结构（五部分）

| 部分 | 章 | 主题 |
|------|-----|------|
| **I** | 1–8 | 基础语言特性 |
| **II** | 9–14 | 模板特性 |
| **III** | 15–20 | 新库组件 |
| **IV** | 21–28 | 库扩展与修改 |
| **V–VI** | 29–35 | 专家工具、编译设置、弃用移除 |

## 章节目录

| 章 | 目录 | 英文标题 |
|----|------|----------|
{chr(10).join(rows)}

## 其他组成部分

- [前言 Preface](./preface/)

## 推荐阅读顺序

1. **语言层速通**：1（结构化绑定）→ 2 → 6 → 9–11（CTAD / `if constexpr` / 折叠表达式）
2. **HFT 高频库**：19（`string_view`）→ 22（并行 STL，配合 08）→ 31（`to_chars`）→ 29（PMR）
3. **类型安全与配置**：15–17（`optional` / `variant` / `any`）→ 20（filesystem 配置/日志路径）
4. **收尾**：34（`-std=c++17` 等设置）→ 35（弃用特性）→ 进入 [10 C++20](../10-C++20-The-Complete-Guide/)

## 学习进度

{progress}
- [ ] 前言
"""
    (ROOT / "README.md").write_text(readme, encoding="utf-8")
    print(f"Created {ROOT} with {len(CHAPTERS)} chapters")


if __name__ == "__main__":
    main()
