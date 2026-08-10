# 12.4 静态分析工具 (Smatch / Sparse)

> ⬜ 跳读

## 本节要点

### Sparse (C 语法检查器)

```bash
# 安装
sudo apt install sparse

# 使用
make C=1 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
# C=1: 只检查修改的文件
# C=2: 检查所有文件

# 常见检测:
# - __user 指针检查
# - __rcu 标注检查
# - 位域可移植性
# - 锁平衡检查
```

### Smatch (语义分析)

```bash
# 安装
git clone https://github.com/error27/smatch.git
cd smatch
make

# 使用
smatch_scripts/build_kernel.sh
smatch_scripts/check_kernel.sh /path/to/linux-source
```

### 检测的问题类型

| 工具 | 检测内容 |
|------|---------|
| Sparse | __user 指针误用、RCU 标注、位域 |
| Smatch | 空指针解引用、未初始化变量、锁不平衡、缓冲区溢出 |

### HFT 关联

静态分析可在编译时发现潜在问题，无需运行代码。HFT 内核模块应在 CI 中加入 Sparse/Smatch 检查。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `__user` 标注的作用是什么？Sparse 如何检查？

> `__user` 标注标记用户空间指针（来自用户空间的指针，不能直接解引用）。Sparse 检查是否在内核代码中直接解引用了 `__user` 指针（应用 `copy_from_user` / `copy_to_user`），以及是否将内核指针泄露给用户空间。


**Q:** Sparse 和 Smatch 分别检测什么类型的问题？

> Sparse：检测类型不匹配（__user/__iomem 指针混用）、RCU 注解违规、锁注解不平衡。基于 GCC 前端做静态分析。Smatch：检测空指针解引用、缓冲区溢出、未检查返回值等。Smatch 更深入做过程间分析但慢。两者互补，CI 中都应运行。

</details>

## 交叉引用

- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/section-5-2.md)
