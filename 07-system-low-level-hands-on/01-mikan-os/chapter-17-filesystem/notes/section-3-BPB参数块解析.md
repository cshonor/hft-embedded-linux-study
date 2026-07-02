## 3. BPB 参数块解析

---

### 一、BPB 在哪

**分区第一个扇区**（引导扇区）内嵌 **BIOS Parameter Block (BPB)** — 描述 **本卷几何与 FAT 参数**。

**不解析 BPB → 无法** 算簇字节偏移 · 找根目录 · 读文件。

---

### 二、关键字段（示意）

| 字段 | 用途 |
|------|------|
| **`bytes_per_sector`** | 每扇区字节数（常 512） |
| **`sectors_per_cluster`** | 每簇扇区数 |
| **`num_fats`** | FAT 表份数 |
| **`sectors_per_fat`** | 单个 FAT 占扇区数 |
| **`total_sectors`** | 卷总扇区（容量） |
| **`root_dir_cluster`** | **根目录起始簇号**（FAT32 关键） |

> FAT12/16 根目录有时为 **固定扇区区**；**FAT32** 根目录为 **普通簇链** — 书中按 **FAT32 + 起始簇** 讲解。

---

### 三、从 BPB 到字节地址

```
cluster N 的字节偏移 ≈
    data_region_base
  + (N - 2) × bytes_per_sector × sectors_per_cluster
```

| 步骤 | 说明 |
|------|------|
| 1 | 读 **引导扇区** → 填充 `BPB` 结构体 |
| 2 | 算 **FAT 区结束** → **数据区起点** |
| 3 | 用 **`root_dir_cluster`** 定位 **根目录第一个簇** |

**内核持有：** 指向 **预读卷镜像** 的基址 + 解析好的 **BPB 全局/单例**。

---

### 四、结构体映射

```cpp
struct BPB {
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    // … num_fats, sectors_per_fat, total_sectors …
    uint32_t root_dir_cluster;  // FAT32
};
```

**注意：** 小端 · 字段偏移与 **Microsoft FAT 规范** 对齐 — 用 `#pragma pack` 或手动偏移读取。

---

← [2. FAT 选型](./section-2-文件系统与FAT选型.md) · 下一节 [4. 目录项](./section-4-目录条目结构.md)
