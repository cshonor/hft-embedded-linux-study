## 6. 块设备驱动与中断处理

---

### 一、`block_device` 描述符

统一表示 **块设备** — 整盘或 **分区**：

- 链接到 **`gendisk`**  
- 与 VFS inode、主次设备号 对应  

不管 `/dev/sda` 还是 `/dev/sda3`，同一套抽象。

---

### 二、驱动初始化（概要）

块驱动使用前通常：

1. 分配 / 初始化 **`gendisk`**  
2. 注册 **主设备号**（→ [Ch 13 动态分配](../chapter-13-io-architecture/notes/section-4-设备文件.md)）  
3. 设置 **`media_changed`** 等底层方法（可移动介质换盘检测）  
4. 注册到 **设备驱动模型**（sysfs / udev）  

---

### 三、DMA 完成与中断

传输由 **DMA** 执行（→ [Ch 13 DMA](../chapter-13-io-architecture/notes/section-5-驱动通用特性.md)）；结束时 **磁盘控制器发 IRQ**：

```
IRQ → 块驱动 interrupt handler
    ↓
end_that_request_first()  — 更新 request 进度
end_that_request_last()   — 完成 request、释放 bio
    ↓
唤醒等待该 I/O 的进程；派发队列中下一 request
```

→ IRQ 框架：[Ch 4 section-6](../chapter-04-interrupts-and-exceptions/notes/section-6-IO中断处理.md)

> **深潜可选：** I/O 完成 **回调**（`bio_endio`）— 页缓存写回、read 填充页 在此衔接 [Ch 15](../chapter-15-page-cache.md)。

---

### 四、本章小结

```
read/write → VFS → 映射层 → bio
    ↓
request_queue + elevator（合并/排序）
    ↓
块驱动 + DMA
    ↓ IRQ 完成
唤醒进程 / 触发 bio 回调
```

多数 **读** 在到达块层前已被 **页缓存** 命中 — [Ch 15](../chapter-15-page-cache.md)。

---

### 五、后续章节索引

| Ch 14 主题 | 继续读 |
|------------|--------|
| 页高速缓存 | [Ch 15 页缓存](../chapter-15-page-cache.md) ⚪ |
| 文件 read 完整路径 | [Ch 16 文件访问](../chapter-16-file-access.md) ⚪ |
| VFS | [Ch 12](../chapter-12-VFS/) ⚪ |
| 字符 / DMA / sysfs | [Ch 13](../chapter-13-io-architecture/) ⚪ |
| 页回收 / 写回 | [Ch 17 页回收](../chapter-17-page-reclaim.md) 🟡 |

---

← [5. I/O 调度](./section-5-IO调度程序.md) · 下一章 [Ch 15 页缓存](../chapter-15-page-cache.md)
