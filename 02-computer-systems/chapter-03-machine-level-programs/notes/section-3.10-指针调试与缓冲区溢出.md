## 3.10 指针、gdb 与缓冲区溢出

### 3.10.1 理解指针

- 指针值 = **地址**；解引用 = 内存 load/store
- **野指针、use-after-free** — 机器级无区别，照样 `mov` 到非法地址 → segfault 或更糟

### 3.10.2 gdb 调试器（实操）

```bash
gdb ./a.out
(gdb) break main
(gdb) run
(gdb) disassemble          # 看汇编
(gdb) info registers
(gdb) x/10gx $rsp          # 检查栈
(gdb) bt                   # 回溯
```

**HFT：** 生产裸机多用 **core dump + gdb 离线** 或 `perf`；开发期 `-g` 保留符号。

### 3.10.3 内存越界与缓冲区溢出

```c
void vuln() {
    char buf[8];
    gets(buf);  // 无边界检查 → 覆盖栈上返回地址
}
```

- 覆盖 **返回地址** → `ret` 跳到攻击者代码 — 经典栈溢出
- 同样逻辑适用于 **堆溢出**、**off-by-one**

**HFT  relevance：** 解析 **不可信** 行情/文本协议时，固定缓冲 + 长度检查；C++ 用 `string_view` / Rust 用切片 — 热路径也要 **bound check 或证明安全**。

### 3.10.4 对抗缓冲区溢出

| 机制 | 作用 |
|------|------|
| **栈 canary** (`__stack_chk_fail`) | 检测返回地址前 cookie 被改 |
| **NX / DEP** | 栈不可执行 |
| **ASLR** | 地址随机化，难猜跳转目标 |
| **Fortify / FORTIFY_SOURCE** | 编译期替换危险函数 |

- 现代 exploit 链组合绕过 — **根本办法：不写越界**（内存安全语言、模糊测试）

### 3.10.5 变长栈帧

- `alloca`、VLA、`clang` 动态栈分配 — 函数尾声才调整 `%rsp`
- 与 **split stack / 大栈** 配置相关 — 深层框架服务少见，HFT 服务注意 **ulimit -s**

---

← [本章导读](../README.md)
