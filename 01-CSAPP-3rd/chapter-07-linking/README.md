# Ch 7 链接 · Linking

> **CSAPP 3rd** · Bryant & O'Neill · **选读 🟡**（Part II）

> 本章定位：**`.o` 怎么拼成可执行文件** — ELF、符号解析、重定位、静态/动态库、PIC。HFT 日常不手写链接脚本，但 **构建一致性、启动延迟、`undefined reference`、静态 vs 动态** 都绕不开链接。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 7.1 编译器驱动程序 | [notes/section-7.1-编译器驱动程序.md](./notes/section-7.1-编译器驱动程序.md) |
| 7.2 静态链接 | [notes/section-7.2-静态链接.md](./notes/section-7.2-静态链接.md) |
| 7.3 目标文件 | [notes/section-7.3-目标文件.md](./notes/section-7.3-目标文件.md) |
| 7.4 可重定位目标文件 | [notes/section-7.4-可重定位目标文件.md](./notes/section-7.4-可重定位目标文件.md) |
| 7.5 符号和符号表 | [notes/section-7.5-符号和符号表.md](./notes/section-7.5-符号和符号表.md) |
| 7.6 符号解析与静态库 | [notes/section-7.6-符号解析与静态库.md](./notes/section-7.6-符号解析与静态库.md) |
| 7.7 重定位 | [notes/section-7.7-重定位.md](./notes/section-7.7-重定位.md) |
| 7.8 可执行目标文件 | [notes/section-7.8-可执行目标文件.md](./notes/section-7.8-可执行目标文件.md) |
| 7.9 加载可执行目标文件 | [notes/section-7.9-加载可执行目标文件.md](./notes/section-7.9-加载可执行目标文件.md) |
| 7.10 动态链接共享库 | [notes/section-7.10-动态链接共享库.md](./notes/section-7.10-动态链接共享库.md) |
| 7.11 从应用程序加载共享库 | [notes/section-7.11-从应用程序加载共享库.md](./notes/section-7.11-从应用程序加载共享库.md) |
| 7.12 PIC位置无关代码 | [notes/section-7.12-PIC位置无关代码.md](./notes/section-7.12-PIC位置无关代码.md) |
| 7.13 库打桩 | [notes/section-7.13-库打桩.md](./notes/section-7.13-库打桩.md) |
| 7.14 处理目标文件的工具 | [notes/section-7.14-处理目标文件的工具.md](./notes/section-7.14-处理目标文件的工具.md) |
| 7.15 小结 | [notes/section-7.15-小结.md](./notes/section-7.15-小结.md) |


---

## 大白话 · 本章一条线

> **编译器产出拼图块 (`.o`)，链接器按符号表拼成完整程序，加载器再塞进内存跑。**

```
.c/.cpp  →  .o (REL)  →  a.out (EXEC)  →  execve 映射进 VM
              ↑ 符号解析 + 重定位
         静态库 .a / 动态库 .so
```

**HFT 三件事：**

1. **生产与回测同一链接方式** — 静态/`RPATH`/glibc 版本一致，避免「实验室快、上线慢」
2. **启动路径** — 动态库 + `dlopen` 插件 vs 单静态 blob（延迟、页 fault）
3. **排错** — `nm`/`readelf`/`ldd` 读懂 undefined reference、ODR、weak symbol

---

## 本章 Checklist

- [ ] 说出 **可重定位 / 可执行 / 共享** 三类 ELF 目标文件
- [ ] 区分 **强符号 vs 弱符号**；多重定义如何报错
- [ ] 解释静态库 **按 archive 成员逐条拉入** 的规则
- [ ] 读懂 **重定位条目** 在修什么（S+A-P 公式直觉）
- [ ] 描述 **动态链接**：PLT/GOT、延迟绑定、`LD_LIBRARY_PATH`
- [ ] 说明 **PIC** 为何共享库必须位置无关
- [ ] 会用 `objdump -t/-d`、`readelf -hS`、`nm`、`ldd`

---

## HFT 精读捷径

```
必读切片：7.2 静态 vs 7.10 动态 + 7.12 PIC + 7.14 工具
构建踩坑：7.6 符号解析
与 VM 衔接：7.9 → Ch 9
7.13 打桩：调试/Profiling 时用，非热路径
```

---

## 相关章节

- 上一章：[../chapter-06-memory-hierarchy/](../chapter-06-memory-hierarchy/)
- 下一章：[../chapter-08-exceptional-control-flow/](../chapter-08-exceptional-control-flow/)
- 编译入门：[../chapter-01-tour-of-computer-systems/](../chapter-01-tour-of-computer-systems/)
- 虚拟内存加载：[../chapter-09-virtual-memory/](../chapter-09-virtual-memory/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
