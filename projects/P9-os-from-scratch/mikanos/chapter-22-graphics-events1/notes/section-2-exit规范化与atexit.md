## 2. exit 规范化与 atexit

---

### 一、从 SyscallExit 到 exit()

**Ch21：** 应用可能 **直接调底层 `SyscallInvoke(EXIT)`**。

**Ch22 规范：**

```c
#include <stdlib.h>

void foo() {
    atexit(cleanup_handler);
    exit(0);    // 不再直接 SyscallExit()
}
```

| 原因 | 说明 |
|------|------|
| **C 标准语义** | `exit()` 应 **flush stdio · 调 atexit 链 · 再 _exit** |
| **应用可移植性** | 与 **Newlib/POSIX** 习惯一致 |
| **资源清理** | 注册 **析构/关闭** — 后续 GUI **关窗前的局部清理** |

---

### 二、Newlib exit 路径

```
exit(code)
  → __call_exitprocs()   // atexit 逆序
  → _exit(code)
      → SyscallInvoke(0x80000002, code)   // 内核 CallApp 恢复
```

→ [Ch21 exit syscall](../chapter-21-window-apps/notes/section-4-exit系统调用与CallApp栈恢复.md)

---

### 三、与 CloseWindow 配合

**Ch22 末：** 应用 **ReadEvent 循环** 收到 **kQuit** 后：

```c
CloseWindow(layer_id);
exit(0);   // atexit 仍可跑 · 内核清页表
```

**避免：** 只 **exit** 不 **CloseWindow** → **窗口残骸**（本章 **CloseWindow** 专节）。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. DoWinFunc](./section-3-WinFillRectangle与DoWinFunc.md)
