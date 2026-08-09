# Ch 3 §3.4 常用操作：readelf · QEMU · GDB

> **MikanOS** · 原书第 3 章

## 1. 查看 ELF 头 — 魔数、架构、入口

```bash
readelf -h kernel.elf
```

| 关注字段 | 含义 |
|----------|------|
| **Class** | ELF32 / **ELF64** |
| **Machine** | x86-64 等 |
| **Entry point address** | **`e_entry`** — Loader 跳转目标 |
| **Start of program headers** | Program Header Table 偏移 |

---

## 2. 查看加载段 — **Bootloader 加载依据**

```bash
readelf -l kernel.elf
```

对应 [§3.2 执行视图](./section-3-2-ELF三大结构与链接执行双视图.md) 的 **Program Header**：

```
Type    Offset   VirtAddr   PhysAddr   FileSiz  MemSiz   Flg
LOAD    0x001000 0x00100000 0x00100000 0x00xxx  0x00xxx  R E
LOAD    0x00xxxx 0x001xxxxx 0x001xxxxx 0x00xxx  0x00xxx  RW
```

| 列 | Loader 怎么用 |
|----|---------------|
| **VirtAddr / PhysAddr** | 拷到哪 |
| **Offset** | 从文件哪读 |
| **MemSiz > FileSiz** | 多出来通常是 **.bss** — 要清零 |

---

## 3. 符号表 — 函数 / 全局变量地址

```bash
readelf -s kernel.elf
nm kernel.elf
```

**gdb 断 `KernelMain`** 依赖这里的符号 — 编译要 **`-g`**，别轻易 **`strip`**。

---

## 4. 节头 — 链接 / 调试视图

```bash
readelf -S kernel.elf
```

看 `.text`、`.data`、`.bss`、`.debug_info` 等 — **Loader 运行时不必读**。

---

## 5. QEMU 直接加载 ELF（无需转 bin）

```bash
# 教学 32 位示例
qemu-system-i386 -kernel kernel.elf -d int -no-reboot

# x86-64 UEFI 场景以 Mikan 官方 QEMU 脚本为准（OVMF + 虚拟 U 盘）
```

**`-kernel`** 让 QEMU **内置 Loader** 读 Program Header — 与 MikanLoader 逻辑 **同类**。

---

## 6. GDB 调试内核

```bash
# 终端 1：QEMU 挂起等 GDB
qemu-system-i386 -kernel kernel.elf -s -S

# 终端 2
gdb kernel.elf
(gdb) target remote localhost:1234
(gdb) break KernelMain
(gdb) continue
```

| 要求 | |
|------|---|
| **`kernel.elf` 带符号** | 编译 **`-g`** |
| **未 strip** | 或保留 **debug 专用副本** |

---

## 7. 减小体积（量产前）

```bash
strip kernel.elf
# 仅去调试信息，保留符号表
objcopy --strip-debug kernel.elf kernel_stripped.elf
```

| 操作 | gdb 还能用吗 |
|------|-------------|
| **`strip`** 全部 | ❌ 符号可能没 |
| **`--strip-debug`** | ⚠️ 部分信息仍在 |

**开发期 MikanOS：** 保留 **完整 `kernel.elf`** 调试；量产才考虑 `.bin` / strip。

---

← [3.3 链接脚本](./section-3-3-编译链接脚本与生成流程.md) · 下一节 [3.5 对比与排错](./section-3-5-与vmlinux对比及常见问题.md)
