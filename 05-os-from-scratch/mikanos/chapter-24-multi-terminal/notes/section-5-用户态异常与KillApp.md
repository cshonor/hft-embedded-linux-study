## 5. 用户态异常与 KillApp

---

### 一、旧行为：应用异常拖死 OS

**Ch20 异常 handler：** 打印寄存器 → **Halt/Panic**。

| 触发 | 后果 |
|------|------|
| 应用 **除零** · **野指针 #PF** | **整个 OS 冻结** |
| 内核 **真 bug** | 同样 panic — **合理** |

**多应用时代不可接受** — 需 **只杀肇事应用**。

---

### 二、CPL 检查

**异常栈帧 / CS 中 **CPL**（Current Privilege Level）：**

```cpp
void ExceptionHandler(InterruptFrame* frame) {
    int cpl = frame->cs & 3;
    if (cpl == 3) {
        KillApp(current_app_task, frame);
        return;
    }
    // Ring 0: 内核真故障 — 仍 panic/dump
    KernelPanic(frame);
}
```

| cpl | 含义 |
|-----|------|
| **3** | **用户态** 应用出事 |
| **0** | **内核** 出事 — **不可恢复路径** |

→ [Ch20 Ring3](../chapter-20-syscall/notes/section-2-Ring3与页表User位.md) · [Ch20 异常 dump](../chapter-20-syscall/notes/section-4-异常处理与调试.md)

---

### 三、KillApp() / ExitApp()（汇编）

**目标：** 类似 **Ch21 exit** — **恢复 CallApp 保存的内核 RSP/寄存器** — 但 **非正常退出**。

```nasm
KillApp:
    ; 不 sysret 回崩溃应用
    mov rsp, [task->kernel_rsp_saved]
    ; 恢复 callee-saved
    jmp ExitAppCleanup
```

```cpp
void ExitAppCleanup(Task* t) {
    CleanPageMaps(t->app_pml4);
    t->app_pml4 = nullptr;
    // 终端打印 "app killed" · 回到 TaskTerminal 循环
}
```

| 步骤 | 效果 |
|------|------|
| **强制栈切换** | **离开** 用户 **坏栈** |
| **CleanPageMaps** | **释放 PML4/物理页** |
| **回到 Terminal** | OS **继续服务其他终端** |

---

### 四、与 exit syscall 对比

| | **exit()** | **KillApp** |
|---|------------|-------------|
| 触发 | 应用 **自愿** | **#PF/#DE/#GP** 等 |
| 清理 | **atexit + CloseWindow**（应用负责） | 内核 **强制回收** |
| 用户感知 | 正常结束 | **异常终止** |

---

← [4. noterm](./section-4-窗口层级Bug与noterm.md) · 下一节 [6. 小结](./section-6-小结与后续.md)
