## 5. syscall.h 与窗口系统调用

---

### 一、syscall.h 工程化

**随 syscall 增多，提取公共头：**

```cpp
// apps/syscall.h
#define SYS_PUT_STRING   0x80000000
#define SYS_OPEN_WINDOW  0x80000001   // 示意编号
#define SYS_EXIT         0x80000002
#define SYS_WIN_WRITE    0x80000003

int64_t SyscallInvoke(int64_t id, ...);
void PutString(const char* s);
void exit(int code) __attribute__((noreturn));
LayerId OpenWindow(int w, int h, const char* title);
void WinWriteString(LayerId id, int x, int y, uint32_t color, const char* s);
```

| 收益 | 说明 |
|------|------|
| **编号集中** | 内核/应用 **同步** |
| **汇编 stub 一份** | 各 app **链接同一 `syscall.o`** |
| **后续 Ch22+** | 加 **鼠标/键盘 syscall** 只改头表 |

---

### 二、OpenWindow

```cpp
int64_t SyscallOpenWindow(int w, int h, const char* title) {
    auto* layer = layer_manager->CreateWindow(w, h, title);
    AssociateLayerWithCurrentTask(layer);
    return layer->ID();
}
```

| 参数 | 内核行为 |
|------|----------|
| **w, h, title** | 创建 **ToplevelWindow** · 注册 **Layer** |
| **返回值** | **图层 ID** — 应用后续 **绘制/事件** 句柄 |

→ [Ch9 LayerManager](../chapter-09-layers/) · [Ch10 Window](../chapter-10-window/)

---

### 三、WinWriteString

```cpp
int64_t SyscallWinWriteString(LayerId id, int x, int y,
                              uint32_t color, const char* s) {
    auto* win = GetWindowByLayerId(id);
    // 权限：仅允许写 **本任务拥有** 的 layer
    win->Writer().WriteString(x, y, s, color);
    SendRedraw(layer);
    return 0;
}
```

| 字段 | 说明 |
|------|------|
| **layer ID** | **OpenWindow 返回** |
| **x, y, color** | 客户区 **像素坐标** · **ARGB** |
| **安全** | 校验 **ID 归属当前任务** — 防 **跨应用画屏** |

---

### 四、syscall_table 扩展

```cpp
syscall_table[SYS_PUT_STRING] = SyscallPutString;
syscall_table[SYS_OPEN_WINDOW] = SyscallOpenWindow;
syscall_table[SYS_EXIT] = SyscallExit;
syscall_table[SYS_WIN_WRITE] = SyscallWinWriteString;
```

---

← [4. exit](./section-4-exit系统调用与CallApp栈恢复.md) · 下一节 [6. winhello](./section-6-winhello与小结.md)
