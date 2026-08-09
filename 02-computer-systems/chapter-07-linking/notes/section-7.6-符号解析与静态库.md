## 7.6 符号解析与静态库（7.6.1–7.6.3）

### 7.6.1 多重定义符号

链接器规则（C 语言）摘要：

| 情况 | 处理 |
|------|------|
| 多个 **强符号** 同名 | **错误** |
| 一强一弱 | 选强 |
| 多个弱符号 | 任选其一 |
| `static` 文件局部 | 不冲突 |

```c
// 错误：两个 .c 都定义 int x;
// 正确：一个定义，其他 extern int x;
```

**函数** 必须是强符号；**未初始化全局** 是弱符号（尽量避免）。

### 7.6.2 与静态库链接

- **静态库** = `.a` = **归档的 `.o` 集合**（`ar` 打包）
- 如 `libc.a`、`libfoo.a`

```bash
ar rcs libfoo.a foo.o bar.o
gcc main.o -L. -lfoo -o prog
```

### 7.6.3 链接器如何使用静态库

**关键：** 链接器 **按命令行顺序** 扫描；对 `.a` **仅当当前 undefined 符号需要时才拉入成员**，且 **每个成员最多拉一次**。

```
gcc -L. -lfoo main.o   # 错误顺序：main.o 在前时 -lfoo 可能太晚
gcc main.o -L. -lfoo   # 推荐：需要符号的 .o 在前，库在后
```

**技巧：** 同一库写两次 `-lfoo -lfoo`，或 `--start-group` / `--end-group`。

**HFT：** 大型 monorepo 用 **Bazel/CMake** 管依赖顺序；静态链第三方（fmt、yaml）减少运行时 `.so` 漂移。

### 自测题

<details>
<summary>1. 链接器如何处理静态库（.a 文件）的符号解析？</summary>

链接器按**命令行顺序**扫描目标文件和库。维护三个集合：E（已加入可执行）、U（未解析引用）、D（已定义符号）。遇到 `.a` 文件时，扫描其成员 `.o`，如果某成员定义了 U 中的符号，就把该成员加入 E。**顺序很重要**——库必须在引用它的目标文件之后。`gcc main.o -lm` 正确，`gcc -lm main.o` 可能链接失败。

</details>

<details>
<summary>2. 为什么 `gcc main.o -lm -lm` 有时能解决链接问题？</summary>

静态库只按需提取成员——如果 libA 依赖 libB 的符号，但 libA 在命令行中排在 libB 之后，链接器已扫过 libB 不会回头。重复 `-lm` 或用 `--start-group ... --end-group` 让链接器循环扫描解决循环依赖。HFT 常用 `Wl,--start-group` 处理复杂的静态库依赖链。

</details>


---

← [本章导读](../README.md)
