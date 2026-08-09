## 4. exit 系统调用与 CallApp 栈恢复

---

### 一、Ch20 遗留：应用如何结束？

| 旧行为 | 问题 |
|--------|------|
| **`main` return** 或 **死循环** | 无法 **干净回到 OS** |
| **`sysret` 回用户** | **exit 后不应再回应用** |

**需要：** **`exit(code)`** — 类似 **POSIX `_exit`**。

---

### 二、syscall 编号 `0x80000002`

```c
void exit(int code) {
    SyscallInvoke(0x80000002, code);
    __builtin_unreachable();
}
```

**内核 `SyscallExit`：** **不执行 `sysret`** — **直接恢复 CallApp 保存的内核上下文**。

---

### 三、CallApp() 保存的上下文

**启动应用前（内核 TaskTerminal / CallApp）：**

```cpp
struct AppContext {
    uint64_t kernel_rsp;
    uint64_t saved_regs[…];   // 返回事件循环所需
    int exit_code;
};

void CallApp(ELFApp& app) {
    AppContext ctx;
    SaveKernelStack(&ctx);     // 当前内核 RSP 等
    SwitchToUser(app);         // CR3 · Ring3 · jump entry
    // —— 不应正常返回到这里 ——
    RestoreKernelStack(&ctx);  // exit syscall 路径
    return ctx.exit_code;
}
```

| **exit 路径** | 操作 |
|---------------|------|
| 1 | 记录 **exit_code** |
| 2 | **`mov rsp, ctx.kernel_rsp`** |
| 3 | 恢复 **RBX/RBP…**（按 ABI） |
| 4 | **`ret`** 到 **CallApp 调用者** — 事件循环继续 |

**关键：** 应用 **用户栈/页表** 在 **CallApp 外层** 统一 **CleanPageMaps**（Ch19）。

→ [Ch19 CleanPageMaps](../chapter-19-paging/notes/section-6-CleanPageMaps与小结.md)

---

### 四、与 Linux 对照

| MikanOS | Linux |
|---------|-------|
| **手动保存 kernel RSP** | **内核栈 per-task** · **schedule 切换** |
| **exit → 回到 CallApp** | **do_exit → schedule → 其他任务** |

**语义一致：** 应用 **不再占用 CPU** · **控制权回 OS**。

---

← [3. printf](./section-3-PutString与printf适配.md) · 下一节 [5. 窗口 syscall](./section-5-syscall.h与窗口系统调用.md)
