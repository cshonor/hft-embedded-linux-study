# 第 1 章 · 开始制作编译器 · §3 使用 C♭ 编译器进行编译

← [本章目录](./README.md) · 上一节：[02-four-compiler-stages.md](./02-four-compiler-stages.md) · 下一节：---

## 运行环境

| 要求 | 说明 |
|------|------|
| **OS** | **Linux**（与目标 ELF 一致） |
| **JRE** | **Java 1.5+** — cbc 用 Java 实现，需运行时 |
| **工具链** | 汇编器、链接器等（书中随 cbc 调用或系统 GNU 工具） |

**Windows 读者**：WSL / 虚拟机 Linux 与书一致；概念可迁，环境建议对齐。

---

## 编译演示：Hello, World!

```text
# 概念流程
cbc hello.cb    →    生成可执行文件 hello
./hello         →    Hello, World!
```

| 点 | 说明 |
|----|------|
| **一条命令** | `cbc` 封装狭义编译 + 汇编 + 链接 |
| **源扩展名** | `.cb`（C♭ 源文件） |
| **产物** | 当前目录 **`hello`** — 原生 ELF，非 `.class` |

与 **gcc** 对比：

```text
gcc hello.c -o hello    # C  toolchain
cbc hello.cb            # C♭ toolchain（cbc 自管后续步骤）
```

---

## 读者应建立的心智模型

```text
  hello.cb  ──cbc──→  [内部四阶段]  ──→  hello (ELF)
                                              ↓
                                         Linux loader 执行
```

- **ch2** 将展开 **C♭ 语法** 与 **cbc 源码目录**
- **ch3 起** 按四阶段 **亲手实现** 各 Pass

---

## 与 Rust 读者

| 本书 | Rust |
|------|------|
| `cbc hello.cb` | `rustc hello.rs`（前端+MIR+LLVM+链接） |
| 手写 x86 | 默认 LLVM 后端 — 本书价值在 **看清每一环** |

读 cbc 时可对照：`rustc -C save-temps` / `cargo build --verbose` 看中间产物。

---

## 自测

- [ ] cbc 依赖 JRE 的原因
- [ ] `cbc hello.cb` 与 gcc 四阶段各对应哪一段
- [ ] 下一章 ch2 应回答什么问题（C♭ 语法 · cbc 结构）
