## 5. KernelMain 与错误处理

---

### 一、KernelMain 入口约定

Loader 完成 **ELF 加载 + GOP 探测** 后：

```
1. 构造 KernelMain 所需参数（帧缓冲地址、大小…）
2. 按 ABI 设置寄存器 / 栈
3. 跳转到 kernel 入口（或包装函数再调 KernelMain）
4. UEFI Boot Services 期仍可运行 — 完整 ExitBootServices 在更后章节
```

**本章内核能力：**

- 接收帧缓冲 → **绘制彩色图案**（书中实验）
- 初期含 **`hlt` 循环** — 证明控制权已转移

**Loader 与内核边界：**

| Loader 做 | Kernel 做 |
|-----------|-----------|
| 读 `kernel.elf`、分配页 | 假设 **内存与 fb 已就绪** |
| GOP、传参 | **像素逻辑**、未来调度/驱动 |
| UEFI 错误处理 | 内核内 assert/panic（后续完善） |

---

### 二、EFI_STATUS 错误处理

UEFI 函数普遍返回 **`EFI_STATUS`** — 必须检查，不能忽略。

| 值（示意） | 含义 |
|------------|------|
| **EFI_SUCCESS** | 成功 |
| **EFI_OUT_OF_RESOURCES** | 分配页失败 |
| **EFI_NOT_FOUND** | 协议/文件未找到 |
| **EFI_BUFFER_TOO_SMALL** | 缓冲不足（Ch2 GetMemoryMap 已遇） |

**本章模式：**

```c
Status = gBS->AllocatePages(...);
if (EFI_ERROR(Status)) {
    Print(L"AllocatePages failed: %r\n", Status);
    while (1) { }   // 死循环 — 安全停机，避免继续破坏内存
}
```

| 原则 | 说明 |
|------|------|
| **失败即停** | 引导阶段 **无回收器** — 继续跑可能 trample 固件 |
| **打印 Status** | `%r` 等格式化 — 快速定位 |
| **死循环** | 便于 QEMU monitor 检查 **RIP 停在 panic 处** |

→ 生产 OS 会扩展为 **panic 屏幕** — MikanOS 后续章节强化

---

### 三、稳定性与调试

| 问题 | 对策 |
|------|------|
| 静默失败 | **每步检查 EFI_STATUS** |
| 跳转后无输出 | monitor 看 **RIP** + 是否进入 KernelMain |
| GOP 成功但 kernel 黑屏 | 核对 **传参地址** 与内核使用的指针 |

---

← [4. GOP](./section-4-GOP与帧缓冲区.md) · 下一节 [6. 汇编与小结](./section-6-汇编指针与小结.md)
