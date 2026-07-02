## 3. 第一个内核与 ELF 加载

---

### 一、从 UEFI 应用到独立内核

| | **MikanLoader (.efi)** | **kernel.elf** |
|---|------------------------|----------------|
| **格式** | **PE**（UEFI 可执行） | **ELF**（Linux 常用对象/内核格式） |
| **运行环境** | UEFI Boot Services | Loader 跳转后 **自持**（初期极简） |
| **入口** | `EfiMain` | **`KernelMain`**（及 ELF 声明的 **Entry Point**） |
| **首版行为** | 读盘、GOP、加载内核 | C++ 编写 · **`hlt` 死循环**（省电休眠） |

**第一个内核：** 甚至尚未绘图 — 仅证明 **Loader 能跳到一个独立地址空间执行**。

```cpp
extern "C" void KernelMain(/* 帧缓冲参数 — 见 §5 */) {
    while (true) { __asm__("hlt"); }
}
```

| 指令 | 含义 |
|------|------|
| **`hlt`** | Halt — CPU **停在外部中断可唤醒的休眠**，低功耗忙等 |

---

### 二、为何用 ELF

| 原因 | 说明 |
|------|------|
| **与 UEFI PE 分工** | Loader = PE；内核 = **ELF** — 贴近 Unix/Linux 工具链 |
| **链接器输出** | Clang/LLD 生成 **带程序头/节区** 的标准文件 |
| **Loader 需解析** | **Entry Point**、各 **PT_LOAD** 段加载到物理内存 |

→ [Ch1 PE/ELF/COFF](../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md)

---

### 三、Loader 加载流程

```
1. EFI_FILE_PROTOCOL 读取 U 盘上 kernel.elf 到缓冲
2. 解析 ELF Header — 确认魔数、架构 x86-64
3. 遍历 Program Headers — 对每个 LOAD 段：
      gBS->AllocatePages 分配物理页
      拷贝段内容到对应物理地址
4. 取 e_entry → 函数指针
5. 准备 KernelMain 参数（帧缓冲等）
6. 跳转执行 — 不再返回 EfiMain
```

| 步骤 | 失败点 |
|------|--------|
| 读文件 | 文件不存在、FAT 路径错误 |
| 分配页 | **内存不足** — 需 `EFI_STATUS` 处理（§5） |
| 入口无效 | ELF 链接脚本错误 → RIP 乱飞 |

---

### 四、与 02 川合 OS 对照

| | **01 约 Day 4–5** | **MikanOS Ch 3** |
|---|-------------------|------------------|
| 内核格式 | 二进制 flat / 自定义 | **ELF** |
| 加载者 | 引导扇区 / asm | **UEFI MikanLoader** |
| 首屏 | VGA 文本模式 | **GOP 像素帧缓冲** |

---

← [2. QEMU 与寄存器](./section-2-QEMU监视器与寄存器.md) · 下一节 [4. GOP](./section-4-GOP与帧缓冲区.md)
