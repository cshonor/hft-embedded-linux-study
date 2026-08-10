# 7.3 栈回溯 (Call Trace) 分析

> 🔴 精读

## 本节要点

### Call Trace 示例

```
[  123.456900] Call trace:
[  123.456905]  my_driver_write+0x3c/0x100 [my_module]
[  123.456910]  vfs_write+0xf4/0x2b0
[  123.456915]  ksys_write+0x74/0x100
[  123.456920]  __arm64_sys_write+0x20/0x30
[  123.456925]  invoke_syscall+0x4c/0x110
[  123.456930]  el0_svc_common+0x88/0x110
[  123.456935]  do_el0_svc+0x24/0x80
[  123.456940]  el0_svc+0x30/0x80
[  123.456945]  el0t_64_sync+0x84/0x88
```

### 阅读方法

- **从下往上读**：最底部是入口（用户空间 syscall），最顶部是崩溃点
- `[my_module]` 标记表示该函数来自内核模块
- `+0x3c/0x100` 可用 addr2line 定位源码行

### 分析流程

```
1. 确定崩溃函数: my_driver_write+0x3c
2. 用 addr2line 定位源码行
3. 确定调用路径: write syscall → vfs_write → my_driver_write
4. 检查寄存器 x0 = 0 (NULL pointer dereference)
5. 检查是否有 [my_module] 确认是模块代码崩溃
```

### 不完整的 Call Trace

```
[  123.457000] Call trace:
[  123.457005]  my_corrupt_function+0x0/0x0
[  123.457010]  (null)
[  123.457015]  0xffff0000deadbeef
```

- `(null)` 或无意义地址 = **栈被破坏**（缓冲区溢出覆盖了返回地址）
- 需要检查崩溃前的寄存器值和内存操作

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Call Trace 从上到下还是从下往上读？

> 从下往上读（与 x86 相反，ARM64 通常从崩溃点往下打印调用链）。最底部是入口点（如 `el0_svc` = 系统调用入口），最顶部是崩溃点（如 `my_driver_write`）。但某些配置下顺序可能不同，看 `pc` 值确认崩溃函数。

**Q2:** Call Trace 出现 `(null)` 或无意义地址是什么原因？

> 栈被破坏。常见原因：1) 缓冲区溢出覆盖了栈上的返回地址；2) use-after-free 释放了栈上的变量；3) 栈溢出（递归过深）。需要用 KASAN 或 SLUB debug 检测原始的越界操作，而非修复 Call Trace 本身。


**Q:** Call Trace 中 "<0>" 或 "?" 前缀是什么意思？

> "?" 标记的帧是不可靠的——栈上的值碰巧看起来像返回地址，但可能不是真正的调用链。unwinder 标记这些帧让开发者知道哪些是可信的。没有标记的帧是可靠的（通过帧指针验证）。

</details>

## 交叉引用

- [05.6 ch07 addr2line](chapter-07-oops/notes/section-7-4.md)
