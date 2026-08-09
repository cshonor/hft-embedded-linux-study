## 6. C++ 应用、标准库与小结

---

### 一、逆波兰式（RPN）计算器

**示例：** `2 3 +` → 结果 **5**（运算符在操作数 **之后**）。

| 层次 | 实现 |
|------|------|
| **算法** | 栈：遇数 **push** · 遇运算符 **pop 两次运算 push** |
| **语言** | **C++** — `std::vector` / 或 C 数组 |
| **运行** | 编译为 **ELF** · 终端 `> rpn 2 3 +` |

**意义：** 证明 **真实 C++ 应用** 可在 MikanOS 上 **开发 · 加载 · 交互**。

---

### 二、链接标准库

**Makefile 应用侧：**

```makefile
CXX = x86_64-linux-gnu-g++
# 或 llvm 交叉工具链
APP_LDFLAGS = -lc -lc++ -lm …
```

| 库 | 提供 |
|----|------|
| **libc** | `strcmp` · `atol` · `printf`（若/newlib 支持） |
| **libc++** | C++ 标准库子集 |

**不必** 每个应用 **手写 strcmp** — 与 **Linux 用户态** 开发体验靠拢。

**注意：** 应用 **OS 接口** 仍有限 — 复杂 I/O 待 **Ch20 syscall**。

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **簇链 + cat** | **完整读文件** |
| **磁盘执行** | 内置未命中 → **按名加载** |
| **sti 修复** | **hlt 应用不拖死系统** |
| **ELF + argv** | **C++ RPN** · **exit code** |
| **libc/libc++** | **应用工程化** |

```
Ch18 外部程序
    ↓
Ch19 应用分页隔离
Ch20 syscall · 独立进程
Ch21+ GUI 应用生态
```

---

### 四、后续索引

| Ch18 主题 | 继续读 |
|----------|--------|
| 分页 / 隔离 | [chapter-19-paging](../chapter-19-paging/) |
| 系统调用 | [chapter-20-syscall](../chapter-20-syscall/) 🔴 |
| FAT 基础 | [chapter-17-filesystem](../chapter-17-filesystem/) |
| ELF 引导 | [chapter-03-bootloader-display](../chapter-03-bootloader-display/) |

---

← [5. ELF/argv](./section-5-ELF格式与命令行参数.md) · [Ch 17](../chapter-17-filesystem/) · [Ch 18 导读](../README.md)
