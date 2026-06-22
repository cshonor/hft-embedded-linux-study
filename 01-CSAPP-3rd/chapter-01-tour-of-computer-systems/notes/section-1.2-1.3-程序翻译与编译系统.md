## 1.2–1.3 程序翻译与编译系统

### 1.2 程序被其他程序翻译成不同格式

从 `hello.c` 到能在机器上跑，要经过 **编译系统（compilation system）** 多步翻译：

```
hello.c  ──预处理器──►  hello.i（展开 #include/#define）
         ──编译器──►    hello.s（汇编文本）
         ──汇编器──►    hello.o（目标文件，机器码 + 符号表）
         ──链接器──►    a.out（可执行，含 CRT、libc 等）
```

| 阶段 | 输入 | 输出 | 工具 |
|------|------|------|------|
| 预处理 | `.c` | `.i` | `cpp` / `gcc -E` |
| 编译 | `.i` | `.s` | `cc1` / `gcc -S` |
| 汇编 | `.s` | `.o` | `as` |
| 链接 | `.o` + 库 | 可执行 | `ld` / `gcc` |

**静态链接 vs 动态链接：** 可执行文件可嵌入 libc 代码，或运行时加载 `libc.so` — HFT 生产常静态/部分静态以减少依赖与启动不确定性（细节 → [Ch 7 链接](../../chapter-07-链接.md)）。

### 1.3 了解编译系统如何工作是大有益处的

**为何要懂：**

- **优化** — 知道 `-O2/-O3`、LTO、PGO 在改哪一阶段产物
- **调试** — 崩溃栈、符号、行号对应哪层翻译
- **链接错误** — undefined reference、ODR、ABI 不匹配
- **安全/合规** — 可重现构建、Supply chain

**常用命令（记一遍）：**

```bash
gcc -E hello.c -o hello.i    # 只看预处理
gcc -S hello.c -o hello.s    # 到汇编
gcc -c hello.c -o hello.o    # 到目标文件
gcc hello.o -o hello         # 链接
```

**HFT：**

- 策略引擎 **Release 构建 flags**（`-march=native`、LTO）要在 CI 与生产一致
- 链接 **whole-archive** 静态库 vs 动态 `dlopen` 插件 — 影响启动延迟与页 fault（→ [Ch 7](../../chapter-07-链接.md)、[Ch 9](../../chapter-09-虚拟内存.md)）

---

← [本章导读](../README.md)
