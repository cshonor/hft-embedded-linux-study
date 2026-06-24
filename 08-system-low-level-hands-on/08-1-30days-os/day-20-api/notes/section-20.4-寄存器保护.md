## ④ 寄存器保护 · PUSHAD / POPAD

#### Bug

app 用 **ECX 循环** 调 API 打印一串字符 → **只出第一个**。

#### 根因

OS 内 **C 函数** 编译后 **随意用 ECX 等** — API 返回后 **app 寄存器被踩**。

#### 修复

**API 汇编入口：**

```nasm
api_handler:
    PUSHAD          ; 保存 EAX..EDI 等
    ; … 调 C 实现 …
    POPAD           ; 恢复
    IRETD / RETF    ; 返回 app
```

**ABI 规则：** **内核入口保存 caller-saved 全集** — 直到 app 自己约定 calling convention。

**HFT：** syscall **内核不能破坏用户寄存器**（除返回值约定）；**WRONG** → 极难查的 **heisenbug**。

→ [01-CSAPP Ch3 调用约定](../../../../01-CSAPP-3rd/chapter-03-machine-level-programs/)

---
