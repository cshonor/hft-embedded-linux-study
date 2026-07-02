## 5. 可执行格式与执行域

> Linux **不只有一种** 可执行文件格式

---

### 一、ELF（主流）

**Executable and Linkable Format** — 标准原生二进制：

- 内核 **ELF 格式处理器** 解析头、Program Header  
- 映射 **LOAD** 段到 text/data  
- 入口可能是 **动态链接器** `ld-linux.so` 再转用户 `main`  

→ CSAPP：[01 CSAPP-3rd](../../../01-CSAPP-3rd/) Ch 7–8

---

### 二、脚本与 `binfmt_misc`

| 类型 | 机制 |
|------|------|
| **`#!` 脚本** | 读 shebang → exec **解释器**（如 `/bin/bash`） |
| **`binfmt_misc`** | 管理员注册 **magic number** → 自定义处理器 |

例：注册后遇到 **Windows `.exe`** 可自动 **`wine`** 执行。

→ VFS read/open：[Ch 16](../chapter-16-file-access/)

---

### 三、执行域 (Execution Domains / Personality)

切换进程 **personality**（如 `PER_SVR4`、`PER_SOLARIS`）：

- **模拟** 其他 Unix 的 syscall 语义差异  
- 运行 **为别系统编译** 的二进制（配合兼容层）

`search_binary_handler()` 遍历已注册 **binfmt** 链 — 找到第一个能处理的格式。

---

← [4. ptrace](./section-4-执行跟踪ptrace.md) · 下一节 [6. execve](./section-6-execve与全书索引.md)
