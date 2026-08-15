# 静态分析工具 (Smatch / Sparse)

> ⬜ 跳读

## 概念详解

### Sparse (C 语法检查器)

Sparse 是内核专用的 C 语法检查器，检测类型标注相关的问题。

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
# - __iomem 指针检查
```

### Sparse 检测的问题

| 检测项 | 说明 | 示例 |
|--------|------|------|
| `__user` 指针 | 用户空间指针不能直接解引用 | `*user_ptr` → 应该用 `copy_from_user` |
| `__rcu` 标注 | RCU 保护的数据需要正确标注 | `rcu_dereference()` |
| `__iomem` 指针 | I/O 内存不能直接解引用 | `*iomem_ptr` → 应该用 `readl/writel` |
| 锁平衡 | 函数内锁获取/释放是否配对 | `spin_lock` 无 `spin_unlock` |
| 位域 | 位域的可移植性问题 | 跨平台位域布局 |

### Smatch (语义分析)

Smatch 做更深层次的语义分析，检测逻辑错误。

```bash
# 安装
git clone https://github.com/error27/smatch.git
cd smatch
make

# 使用
smatch_scripts/build_kernel.sh
smatch_scripts/check_kernel.sh /path/to/linux-source

# 或单独检查文件
smatch_scripts/kchecker drivers/my_driver.c
```

### Smatch 检测的问题

| 检测项 | 说明 |
|--------|------|
| 空指针解引用 | 可能 NULL 的指针被解引用 |
| 未初始化变量 | 使用前未赋值 |
| 缓冲区溢出 | 数组越界访问 |
| 锁不平衡 | 锁获取/释放不配对 |
| 返回值未检查 | 函数返回值被忽略 |
| 内存泄漏 | 分配后未释放 |
| 整数溢出 | 算术运算可能溢出 |

### Sparse vs Smatch

| 特性 | Sparse | Smatch |
|------|--------|--------|
| 检测类型 | 类型标注 | 语义分析 |
| 速度 | 快 | 慢 |
| 误报率 | 低 | 较高 |
| 深度 | 浅（语法级） | 深（过程间分析） |
| 内核集成 | `make C=1` | 独立工具 |
| 检测内容 | __user/__rcu/__iomem | 空指针/溢出/泄漏 |

### HFT 关联应用

```makefile
# HFT 模块的 Makefile 中加入静态分析
check:
    make C=2 M=$(PWD) modules
    smatch_scripts/kchecker my_hft_module.c

# CI 中自动运行
ci-check:
    sparse my_hft_module.c
    smatch my_hft_module.c
    # 如果有 warning/failure → 阻止合并
```

### __user 标注示例

```c
// 正确: 使用 __user 标注 + copy_from_user
static ssize_t my_write(struct file *f, const char __user *buf,
                        size_t len, loff_t *off) {
    char kbuf[256];
    if (copy_from_user(kbuf, buf, len))  // 正确: 通过 copy_from_user
        return -EFAULT;
    return len;
}

// 错误: 直接解引用 __user 指针
static ssize_t bad_write(struct file *f, const char __user *buf,
                         size_t len, loff_t *off) {
    char c = *buf;  // Sparse 报警! 不能直接解引用 __user 指针
    return len;
}
```

### __rcu 标注示例

```c
// 正确: 使用 __rcu 标注 + rcu_dereference
static struct data __rcu *global_data;

void read_data(void) {
    rcu_read_lock();
    struct data *p = rcu_dereference(global_data);  // 正确
    use(p);
    rcu_read_unlock();
}

// 错误: 直接访问 __rcu 指针
void bad_read(void) {
    struct data *p = global_data;  // Sparse 报警! 需要 rcu_dereference
    use(p);
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `__user` 标注的作用是什么？Sparse 如何检查？

> `__user` 标注标记用户空间指针（来自用户空间的指针，不能直接解引用）。Sparse 检查是否在内核代码中直接解引用了 `__user` 指针（应使用 `copy_from_user` / `copy_to_user`），以及是否将内核指针泄露给用户空间。

**Q2:** Sparse 和 Smatch 分别检测什么类型的问题？

> Sparse：检测类型不匹配（__user/__iomem 指针混用）、RCU 注解违规、锁注解不平衡。基于 GCC 前端做静态分析。Smatch：检测空指针解引用、缓冲区溢出、未检查返回值等。Smatch 更深入做过程间分析但慢。两者互补。

**Q3:** HFT 模块的 CI 中为什么要加入静态分析？

> 静态分析在编译时发现潜在问题，无需运行代码。可以检测出 __user 指针误用、RCU 标注错误、空指针解引用等问题，在开发早期消灭 bug。HFT 模块对正确性要求高，CI 中应运行 Sparse + Smatch。

**Q4:** `__rcu` 标注为什么重要？

> `__rcu` 标注告诉编译器和 Sparse 该指针受 RCU 保护，必须通过 `rcu_dereference()` 访问。直接访问可能读到中间值（指针更新不是原子的）或触发 RCU 读写同步问题。Sparse 检查是否正确使用了 RCU API。

**Q5:** `make C=1` 和 `make C=2` 的区别？

> `C=1` 只检查重新编译的文件（修改过的文件），速度快。`C=2` 检查所有文件，覆盖全面但慢。开发时用 `C=1`（快速反馈），CI 中用 `C=2`（全面检查）。

</details>

## 交叉引用

- [05.6 ch05 KASAN](../../chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch08 LOCKDEP](../../chapter-08-lock-debug/notes/02-lockdep.md)
- [05.6 ch12 内核测试框架](../../chapter-12-misc/notes/02-kselftest-kunit.md)
