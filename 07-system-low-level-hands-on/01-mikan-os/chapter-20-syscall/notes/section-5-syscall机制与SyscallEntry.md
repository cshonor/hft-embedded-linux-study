## 5. syscall 机制与 SyscallEntry

---

### 一、为何不用 call 内核

**Ring 3 无法 `call` Ring 0 函数** — CPU 拒绝 **特权提升**（除 **syscall/int/中断门** 等 **受控入口**）。

**x86-64 正统：** **`syscall` / `sysret`** — 比 **`int 0x80`**（32 位 Linux 遗留）更快。

---

### 二、MSR 配置

| MSR | 作用 |
|-----|------|
| **`IA32_STAR`** | **sysret** 时段寄存器 · **syscall** 时段基址 |
| **`IA32_LSTAR`** | **`syscall` 跳转地址** → **`SyscallEntry`** |
| **`IA32_FMASK`** | 进入内核时 **RFLAGS 屏蔽**（如 IF） |

```cpp
InitializeSyscallTable(SyscallEntry, kernel_cs, kernel_ss, user_cs, user_ss);
// wrmsr(LSTAR, SyscallEntry);
```

**用户侧 stub（示意）：**

```nasm
SyscallInvoke:
    mov eax, syscall_number
    syscall
    ret
```

---

### 三、SyscallEntry（汇编）

**必须汇编 — C 无法保证 **全部** 寄存器契约：**

```
SyscallEntry:
    ; 硬件已切 CPL=0 · 部分寄存器 clobber 规则
    push 用户态保存区（rax, rdi, rsi, …）
    mov rdi, rax          ; syscall 号
    call SyscallDispatcher
    pop …
    sysret
```

| 约定 | 说明 |
|------|------|
| **EAX/RAX** | **系统调用编号** |
| **RDI, RSI, RDX…** | 参数（与 **Linux x86-64 ABI** 类似） |
| **返回值** | **RAX** |

---

### 四、syscall_table 分发

```cpp
using SyscallFn = int64_t(*)(uint64_t, uint64_t, uint64_t, uint64_t);

SyscallFn syscall_table[] = {
    [0x80000000] = SyscallPutString,
    // …
};

int64_t SyscallDispatcher(uint64_t num, …) {
    return syscall_table[num](…);
}
```

**编号空间：** 本书用 **`0x80000000`** 等与 **Linux 号** 区分 — **避免混淆**。

→ [08 TLPI 系统调用](../../../06-The-Linux-Programming-Interface/) · [05 LKD syscall](../../../03-Linux-Kernel-Development/)

---

← [4. 异常调试](./section-4-异常处理与调试.md) · 下一节 [6. 终端打印](./section-6-终端打印syscall与小结.md)
