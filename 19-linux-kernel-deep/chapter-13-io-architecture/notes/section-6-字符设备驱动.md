## 6. 字符设备驱动程序 (Character Device Drivers)

> **顺序字节流** — 内核对设备的最小支持级别之一

---

### 一、字符 vs 块（预告）

| 类型 | 访问模型 | 下一章 |
|------|----------|--------|
| **字符设备** | 顺序 **字节流** — 不可随机寻址（或能力有限） | 本章 |
| **块设备** | **固定大小块**、可 seek、常带缓存 | [Ch 14 块设备](../chapter-14-block-devices/) |

例：键盘、串口、部分控制设备 → 字符；磁盘 → 块。

---

### 二、`cdev` 描述符

每个字符设备在内核由 **`struct cdev`** 描述：

- 关联 **dev_t**（主次号）  
- 指向 **`file_operations`**（open/read/write/ioctl …）

---

### 三、注册设备号

| API | 用途 |
|-----|------|
| **`alloc_chrdev_region()`** | **动态** 申请一段主次号 |
| **`register_chrdev()`** | 较老接口 — 注册 major 与操作表 |

卸载驱动时 **释放** 设备号、删除 `cdev`。

---

### 四、打开设备时 VFS 如何挂钩

```
用户 open("/dev/xxx")
    ↓
VFS 查 inode（字符设备特殊 inode）
    ↓
def_chr_fops（默认字符操作表）
    ↓
根据 inode 中的 cdev → 驱动自定义 file_operations
    ↓
驱动 .open / .read / .write …
```

即 Ch 12 **VFS 多态** 在字符设备上的具体实例。

→ `file_operations`：[Ch 12 section-3](../chapter-12-VFS/notes/section-3-四大核心对象.md)

---

### 五、本章小结

```
硬件（总线 / 控制器 / DMA）
    ↓ 驱动模型（kobject / sysfs）
device + driver + bus
    ↓ /dev 设备文件
VFS open/read/write
    ↓
字符驱动 cdev / 块驱动（Ch 14）
```

---

### 六、后续章节索引

| Ch 13 主题 | 继续读 |
|------------|--------|
| 块设备、I/O 调度 | [Ch 14 块设备驱动](../chapter-14-block-devices/) ⚪ |
| VFS / 设备文件 | [Ch 12 VFS](../chapter-12-VFS/) ⚪ |
| I/O 中断 | [Ch 4 section-6](../chapter-04-interrupts-and-exceptions/notes/section-6-IO中断处理.md) |
| DMA 低端内存 | [Ch 8 页框 ZONE_DMA](../chapter-08-memory-management/notes/section-2-页框管理.md) |
| 文件 read 路径 | [Ch 16 文件访问](../chapter-16-file-access.md) ⚪ |
| LKD 对照 | Linux Kernel Development 设备驱动章节 |

---

← [5. 通用特性](./section-5-驱动通用特性.md) · 下一章 [Ch 14 块设备](../chapter-14-block-devices/)
