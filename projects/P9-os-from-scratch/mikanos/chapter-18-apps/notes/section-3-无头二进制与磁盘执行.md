## 3. 无头二进制与磁盘执行

---

### 一、第一个应用：无头 .bin

**无头（headless）** — 无 **ELF/MZ 头** — 纯 **机器码** 从偏移 0 执行。

```nasm
; 极简示例：死循环 hlt
.loop:
    hlt
    jmp .loop
```

| 特点 | 说明 |
|------|------|
| **汇编直接输出 .bin** | 无链接脚本 · 无 crt0 |
| **入口 = 文件首字节** | OS **读入内存 → 直接 call/jmp** |

---

### 二、终端 fallback 执行

**Ch16 `ExecuteLine()` 扩展逻辑：**

```
解析 cmd + args
    ↓
内置命令? (echo, clear, ls, cat, …)
    ↓ 否
在 FAT 根目录按 cmd 找文件 (8.3 名)
    ↓ 找到
ReadFileClusterChain → load_buffer
    ↓
跳转到 load_buffer 执行
    ↓ 返回
继续终端循环
```

**与 shell 类似：** 命令名 **即程序名** — 尚未 **PATH / shebang**。

---

### 三、加载地址

| 考虑 | 实践 |
|------|------|
| **固定加载区** | 如 **0x100000** — 不与内核重叠 |
| **大小** | 按目录项 **file_size** 或簇链总长 |
| **权限** | 本章 **flat 内存** — 应用与内核 **同特权**（Ch19 分页后改进） |

→ [Ch8 内存布局](../chapter-08-memory/)

---

### 四、执行模型（本章）

```
TaskTerminal 收 Enter
  → ExecuteLine
  → （内置 or 读盘）
  → 直接 call 应用代码
  → 应用 ret 回到终端
```

**尚无独立进程地址空间** — 本质是 **内核线程里调用外来代码** — Ch20 **syscall + 任务** 才 **进程化**。

→ [chapter-20-syscall](../chapter-20-syscall/)

---

← [2. cat](./section-2-FAT簇链与cat命令.md) · 下一节 [4. sti Bug](./section-4-sti与hlt冻结Bug.md)
