# Ch 4 §5 内核与用户空间的数据拷贝

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 5. 内核与用户空间的数据拷贝

内核 **不能** 裸 dereference 用户指针 — 页可能 **未映射、已换出、仅用户可读**。

| API | 方向 |
|-----|------|
| **`copy_from_user()`** | 用户 → 内核 |
| **`copy_to_user()`** | 内核 → 用户 |
| **`get_user()` / `put_user()`** | 标量快捷版 |

**机制：**

1. 正常拷贝；若访问 **无效用户地址** → MMU 异常  
2. **异常表 (Exception Table)** 把 **出错 RIP** 映射到 **fixup 代码**  
3. fixup **返回错误码**，**不 oops 内核**

**HFT：**  syscall / **ioctl** 路径上的拷贝次数与 **用户指针校验** — 网关若 **内核模块** 或 **bpf** 边界，需理解 **为何不能直接用 `memcpy` 对用户指针**。

---
