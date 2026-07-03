## 4. GetMemoryMap 与导出 memmap

---

### 一、调用链概览

```
MikanLoader (EfiMain)
    ↓
gBS  ←  UEFI Boot Services 全局表
    ↓
GetMemoryMap()  — 获取内存描述符数组
    ↓
Simple File System Protocol  — 访问 U 盘
    ↓
EFI_FILE_PROTOCOL  — 创建/写入 memmap 文件
    ↓
CSV 文本写入 U 盘根目录（或指定路径）
```

---

### 二、`gBS->GetMemoryMap()`

| 要素 | 说明 |
|------|------|
| **`gBS`** | **全局 Boot Services 表** 指针 — EDK II 中常用全局变量 |
| **GetMemoryMap** | 填充 **EFI_MEMORY_DESCRIPTOR** 数组 |
| **典型参数** | MapSize、MapKey、DescriptorSize、DescriptorVersion |
| **MapKey** | 映射版本键 — 内存分配后可能变化，后续 **ExitBootServices** 前需一致 |

**输出：** 若干条描述符，每条 = **一段连续物理地址 + 类型 + 属性** —— 即 [§3 四层 RAM 地图](./section-3-主存储器与内存映射.md#二ram-占用四层--uefi-开机后的典型分层) 的 **机器可读清单**。

| CSV 列（概念） | 对应 |
|----------------|------|
| 起始地址、长度 | 这一段 **物理 RAM** 占哪 |
| **Type** | `Conventional` / `LoaderCode` / `ACPI` … |
| **Attribute** | 可执行、可写、缓存策略 … |

**读 CSV 时：** 找 **`LoaderCode`** 行 —— 那是 **MikanLoader 自己的代码段**，属性常含 **可执行、不可写**；找 **`ConventionalMemory`** —— 那是你 **Ch 8 内核该从这里划页** 的候选区。

```c
// 示意 — 实际需按 UEFI 规范处理缓冲区大小与重试
Status = gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey,
                           &DescriptorSize, &DescriptorVersion);
```

**注意：** 首次调用常因缓冲区不足返回 **`EFI_BUFFER_TOO_SMALL`** — 需先查询 `MapSize` 再分配缓冲（书中完整流程）。

---

### 三、写入 U 盘：文件协议

| 协议 | 作用 |
|------|------|
| **Simple File System Protocol** | 挂载 FAT 等卷 |
| **`EFI_FILE_PROTOCOL`** | 打开/创建/读/写 **文件** |

**本章操作：**

1. 定位启动用 U 盘（或 QEMU 虚拟 FAT 盘）
2. 创建文件 **`memmap`**
3. 将每条内存描述符格式化为 **CSV 行** 写入

**CSV 价值：**

| 用途 | 说明 |
|------|------|
| **人工分析** | Excel / 编辑器查看空洞与保留区 |
| **后续开发** | 内核初始化时 **读入** 相同布局（或运行时再次 GetMemoryMap） |
| **调试** | 真机 vs QEMU 内存差异对比 |

---

### 四、ExitBootServices 预告（概念）

OS 正式接管前，Loader 需调用 **`ExitBootServices`** — 此后 **Boot Services（含 GetMemoryMap）不可用**。

| 阶段 | 内存信息 |
|------|----------|
| **Loader 期（本章）** | 随时 `GetMemoryMap` |
| **内核期** | 必须在 Exit 前 **保存** memmap，或自带探测逻辑 |

→ Ch 3+ 引导流程会延续 MikanLoader 职责

---

### 五、实验自检

- [ ] QEMU 运行 MikanLoader 后 U 盘映像中出现 **`memmap`**
- [ ] CSV 中含 **ConventionalMemory** 行 — 地址范围合理
- [ ] 存在 **Reserved / ACPI** 等类型 — 非全盘可用

---

← [3. 内存映射](./section-3-主存储器与内存映射.md) · 下一节 [5. 指针基础](./section-5-C指针基础.md)
