# 《C 专家编程》· 链接与内存

**Expert C Programming: Deep C Secrets** — Peter van der Linden

> **第 3 本书** · 只精读 ch05 链接 + ch06 运行时数据结构 + ch07 内存探险
> **目录改名（2026-09-09）：** 原 `03-expert-declarations-and-linking` → 现 `03-expert-linking-and-memory`，反映实际精读范围（ch05/06/07）。

## 定位

阶段 1 · C 语言进阶。学了 01 K&R + 02 C 和指针 之后，本书只补三块独有内容：
**链接器行为（ch05）+ 运行时数据结构 a.out/段/栈帧（ch06）+ 内存布局深入（ch07）**。
其余章节与 01/02 重复，标 ⏭️ 跳过不再投入。

## 阅读建议

K&R +《C 和指针》之后阅读；只读 🔴 三章即可，帮助从「会写 C」到「懂 C 在机器上怎么跑」。

## 章节索引与重复判定

全书 11 章 + 2 附录。重复判定对照 01 K&R 与 02《C 和指针》。

| 章 | 目录 | 主题 | 策略 | 重复来源 |
|----|------|------|------|----------|
| 第 1 章 | [ch01-c-through-the-mists-of-time](./ch01-c-through-the-mists-of-time/) | C：穿越时空的迷雾 | 🟡 略读 | ANSI 历史独有 |
| 第 2 章 | [ch02-its-not-a-bug-its-a-language-feature](./ch02-its-not-a-bug-its-a-language-feature/) | 这不是 Bug，而是语言特性 | 🟡 略读 | 02 Reek ch05 已讲，03 视角独有 |
| 第 3 章 | [ch03-analyzing-c-declarations](./ch03-analyzing-c-declarations/) | 分析 C 语言的声明 | 🟡 略读 | **02 Reek ch07 已深讲** |
| 第 4 章 | [ch04-arrays-are-not-pointers](./ch04-arrays-are-not-pointers/) | 令人震惊的事实：数组和指针并不相同 | 🟡 略读 | **02 Reek ch08 已深讲** |
| **第 5 章** | [ch05-thinking-of-linking](./ch05-thinking-of-linking/) | 对链接的思考 | 🔴 精读 | 02 不讲链接器 |
| **第 6 章** | [ch06-runtime-data-structures](./ch06-runtime-data-structures/) | 运动的诗章：运行时数据结构 | 🔴 精读 | 02 不讲 a.out/段/栈帧 |
| **第 7 章** | [ch07-adventures-in-memory](./ch07-adventures-in-memory/) | 对内存的思考 | 🔴 精读 | 02 ch18 讲过部分，03 深入度更高 |
| 第 8 章 | [ch08-halloween-vs-christmas](./ch08-halloween-vs-christmas/) | 为什么程序员无法分清万圣节和圣诞节 | ⏭️ 跳过 | 02 Reek 有更标准的优先级表 |
| 第 9 章 | [ch09-more-about-arrays](./ch09-more-about-arrays/) | 再论数组 | ⏭️ 跳过 | **02 Reek ch08 已深讲** |
| 第 10 章 | [ch10-more-about-pointers](./ch10-more-about-pointers/) | 再论指针 | ⏭️ 跳过 | **02 Reek 已深讲** |
| 第 11 章 | [ch11-cpp-for-c-programmers](./ch11-cpp-for-c-programmers/) | 你懂得 C，所以 C++ 不在话下 | ⏭️ 跳过 | C++ 主线相关 |

### 附录

| 附录 | 目录 | 主题 | 策略 |
|------|------|------|------|
| 附录 A | [appendix-a-job-interview-secrets](./appendix-a-job-interview-secrets/) | 程序员工作面试的秘密 | ⏭️ |
| 附录 B | [appendix-b-glossary](./appendix-b-glossary/) | 术语表 | ⏭️ |

> **⏭️ 跳过的章节保留笔记不删**（资产保留，未来交叉引用仍可用）；README 在策略列标明「与 02 Reek 重复」，复习时跳过即可。

## 学习进度

- [x] ~~第 1–4、8–11 章~~（早期全章笔记已写，现按策略不再复习）
- [ ] 🔴 第 5 章 对链接的思考（精读）
- [ ] 🔴 第 6 章 运动的诗章：运行时数据结构（精读）
- [ ] 🔴 第 7 章 对内存的思考（精读）
- [ ] 📖 附录 A/B 工具书（按需查）
