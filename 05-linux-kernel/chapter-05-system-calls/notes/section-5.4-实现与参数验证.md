## ④ 实现与参数验证 · Implementation & Security

---

### 痛点：内存隔离

用户态地址空间 与 内核地址空间 **受保护隔离**。  
内核函数 **不能直接解引用用户态指针**！

例如：

```c
read(fd, buf, 100);
```

`buf` 是用户态虚拟地址（用户进程页表里的映射）。内核若直接 `*buf` 访问：
- 该页可能不在内存 → 触发缺页（错误路径下没人兜底）；
- 地址非法/未映射 → 直接 **Oops** 内核崩溃；
- 恶意程序**故意传伪造指针** → 内核越读越写，直接变成安全漏洞。

---

### 核心函数

| 函数 | 方向 |
|------|------|
| **`copy_from_user()`** | 数据 **用户态 → 内核态** |
| **`copy_to_user()`** | 数据 **内核态 → 用户态** |

这两个函数做三件事：**校验**用户虚拟地址是否属于当前进程、**优雅处理缺页**（页不在内存时安全换入，不 Oops）、**完成拷贝**。
凡是系统调用接收用户传入指针，**必须** 用这一组（或同类安全接口）；直接裸访问会崩溃 / 被当成漏洞。

失败时常返回 **`-EFAULT`**（用户地址非法）。

⚠️ **高频考点：`copy_from_user`/`copy_to_user` 可能睡眠** —— 缺页处理可能要等磁盘 I/O 换页。所以：
- ❌ **中断上下文**（IRQ handler、tasklet、softirq）里禁止调用 → [Ch 7](../../chapter-07-interrupts/)；
- ❌ **持有自旋锁**期间禁止调用 → [Ch 9–10](../../chapter-09-kernel-sync-intro/)（自旋锁持有者不能睡眠）；
- ✅ 只能在**进程上下文**（syscall、内核线程）里用 → [§5.5](./section-5.5-系统调用上下文.md)。

---

### 系统调用函数标准形式

```c
SYSCALL_DEFINE3(read, int, fd, char __user *, buf, size_t, count)
{
    /* __user 只是标记：指针来自用户空间，提醒不能直接解引用 */
}
```

- `SYSCALL_DEFINE0~5`：尾号数字 = **参数个数**，宏自动从寄存器提取参数、包装函数、注册进系统调用表（寄存器传参细节见 §5.2/5.3）。
- `__user` 是 **gcc sparse 静态检查标记**：编译期提示"此指针来自用户空间"，**运行时不产生任何机器码**——它只是给 sparse 和开发者看的，内核里对该指针照样不能写 `*buf`。

---

### 参数校验要点

1. 指针范围合法性；
2. 缓冲区长度合法性；
3. 文件描述符是否有效（在内核里查进程 fd 表）；
4. 权限检查（能不能读、能不能写）。

内核 **必须** 假定用户空间 **恶意或错误**。

---

### 设计原则（书中观点）

| 建议 | 反例 |
|------|------|
| **功能单一** | 一个 syscall 干太多事 |
| 避免「万能 syscall」 | 滥用 **`ioctl()`** 塞无数私有命令（§5.6 仍可用，但要克制） |

---

### 权能 · Capabilities

特权操作不只靠 UID，还用 **`capable()`** 等检查 **细粒度权能**，例如：

| 权能 | 操作示例 |
|------|----------|
| **`CAP_SYS_REBOOT`** | `reboot()` |
| **`CAP_NET_ADMIN`** | 网络配置 |

→ **Ch 9–10** 锁与并发 · **Ch 7** 中断上下文（不可睡眠）对比 · 下一节 [§5.5 上下文](./section-5.5-系统调用上下文.md)


> ↔ [ULK Ch10 §6 参数验证与内核封装](../../../16-linux-kernel-deep/chapter-10-system-calls/notes/section-6-参数验证与内核封装.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** access_ok() 验证什么？为什么不能完全保证安全？

<details><summary>答案</summary>

access_ok(addr, size) 验证 [addr, addr+size) 区间在用户态地址范围内（< TASK_SIZE），防止用户态传入内核地址。但不能保证：1) 页面已映射（可能 page fault）；2) 指针指向的内存有效；3) 竞争条件（TOCTOU：验证后另一个线程 munmap）。完整安全需要 copy_from_user/copy_to_user 配合。

</details>

**Q2.** copy_from_user() 和 memcpy() 的区别？为什么内核不能用 memcpy 拷贝用户态数据？

<details><summary>答案</summary>

memcpy 直接拷贝不检查地址 → 如果用户态传入内核地址会破坏内核内存。copy_from_user 检查地址范围 + 处理 page fault + 返回未拷贝字节数。如果用户态页面被换出，copy_from_user 会安全地 page in，memcpy 会直接 oops。

</details>

**Q3.** ① 内核里能直接解引用 `__user` 指针吗？② `copy_from_user` 失败返回什么？③ `__user` 是运行时还是编译期机制？④ 为什么中断上下文里不能调 `copy_from_user`？

<details><summary>答案</summary>

① 不能——该地址可能未映射（缺页）、非法（Oops），恶意伪造指针还会造成内核越权读写漏洞。② 返回 `-EFAULT`。③ 编译期：gcc sparse 静态检查标记，运行时不产生任何机器码。④ `copy_from_user` 内部可能触发缺页异常，缺页换页可能睡眠；中断上下文没有可被调度的进程上下文，禁止睡眠 → 调用会造成严重 bug。同理，持有自旋锁时也不能调用。

</details>

</details>
---
