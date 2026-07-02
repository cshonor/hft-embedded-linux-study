## 6. 终端打印 syscall 与小结

---

### 一、SyscallPutString — `0x80000000`

**首个实用 syscall：** 把 **用户缓冲区字符串** 打到 **内核终端/Console**。

```cpp
int64_t SyscallPutString(const char* s, …) {
    // 校验用户指针在 User 映射范围内
    PrintToTerminal(s);
    return 0;
}
```

| 要点 | 说明 |
|------|------|
| **指针来自 Ring3** | 内核 **必须验证 VA** — 防 **伪造内核地址** |
| **复用 Ch5/Ch16** | **printk / Terminal** 输出链 |

→ [Ch16 终端](../chapter-16-commands/) · [Ch5 Console](../chapter-05-console-text/)

---

### 二、rpn 应用改造

**修改 RPN 计算器：**

```cpp
// 计算过程中
SyscallInvoke(0x80000000, "push 2\n");
SyscallInvoke(0x80000000, "result: 5\n");
```

**验证：**

```
> rpn 2 3 +
push 2
push 3
+
result: 5
5
```

**意义：** 应用 **不再只有 exit code** — **安全 I/O 通道** 打通。

---

### 三、用户态 C 库 stub

**应用 Makefile 链接：**

```c
// syscalls.hpp
int64_t SyscallInvoke(int64_t id, ...);

#define SYS_PUT_STRING 0x80000000
void puts(const char* s) { SyscallInvoke(SYS_PUT_STRING, s); }
```

**与 Newlib `_write` 对接** — Ch21+ 可 **printf 到终端**。

→ [Ch5 Newlib 铺垫](../chapter-05-console-text/notes/section-5-Console与Newlib.md)

---

### 四、本章总结

| 成果 | 说明 |
|------|------|
| **Ring 3 + User 页** | **内核内存保护** |
| **TSS.RSP0** | **中断内核栈** |
| **异常 dump** | **PF/GP 可调试** |
| **syscall/sysret** | **SyscallEntry + table** |
| **0x80000000** | **终端字符串输出** |

```
Ch20 syscall 基础
    ↓
Ch21 窗口 GUI 应用（更多 syscall）
Ch29 IPC
```

---

### 五、后续索引

| Ch20 主题 | 继续读 |
|----------|--------|
| GUI 应用 | [chapter-21-window-apps](../chapter-21-window-apps/) ⚪ |
| IPC | [chapter-29-ipc](../chapter-29-ipc/) |
| 分页 | [chapter-19-paging](../chapter-19-paging/) 🔴 |
| Linux syscall | [03-Linux-Kernel-Development](../../../03-Linux-Kernel-Development/) · [06-The-Linux-Programming-Interface](../../../06-The-Linux-Programming-Interface/) |

---

← [5. syscall 机制](./section-5-syscall机制与SyscallEntry.md) · [Ch 19](../chapter-19-paging/) · [Ch 20 导读](../README.md)
