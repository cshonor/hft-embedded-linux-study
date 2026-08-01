## 2. FAT 簇链与 cat 命令

---

### 一、为何需要簇链

**Ch17 `ls`：** 读 **目录簇** — 每条 **32B** 含 **起始簇号**。

**读文件内容：** 文件常 **大于一簇** — 数据 **分散** 在多个簇 — 靠 **FAT 表** 链接。

```
文件数据:  [ 簇 5 ] → [ 簇 8 ] → [ 簇 12 ] → EOF
FAT[5]=8  FAT[8]=12  FAT[12]=0x0FFFFFFF (FAT32 EOC)
```

---

### 二、追踪 FAT 表

| 步骤 | 操作 |
|------|------|
| 1 | 目录项 → **first_cluster** |
| 2 | 按 BPB 算 **簇 → 字节偏移**（Ch17 公式） |
| 3 | 读 **data 区** 该簇 → 追加 buffer |
| 4 | **FAT[n]** → 下一簇 · 直到 **End Of Chain** |

**FAT 表位置：** 引导扇区后 · **num_fats × sectors_per_fat** — 从 **内存卷镜像** 索引。

---

### 三、cat 命令

```cpp
if (strcmp(cmd, "cat") == 0) {
    auto entry = FindFile(args);
    ReadFileClusterChain(entry, buffer);
    Print(buffer);   // 文本假设 · 遇 \0 或 file_size 截断
}
```

| 用途 | 验证 **簇链读取** · 终端 **显示文件** |
|------|--------------------------------------|
| 示例 | `> cat README.TXT` |

→ [Ch17 BPB · 目录项](../chapter-17-filesystem/notes/section-4-目录条目结构.md)

---

### 四、与 ls 的分工

| 命令 | 读什么 |
|------|--------|
| **ls** | **根目录** 条目数组 |
| **cat** | **普通文件** 全簇链内容 |

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 无头 bin](./section-3-无头二进制与磁盘执行.md)
