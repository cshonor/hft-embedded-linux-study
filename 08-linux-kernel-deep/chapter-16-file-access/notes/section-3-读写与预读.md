## 3. 读写文件与预读 (Read / Write / Read-Ahead)

---

### 一、读：`generic_file_read()`

VFS 处理 **普通文件读** 的核心路径：

```
read() syscall
    ↓
generic_file_read()
    ↓
在页缓存（address_space 基数树）查页
    ├─ 命中 → 拷贝到用户空间
    └─ 未命中 → 分配页框 → 块层读盘 → 填入页缓存 → 拷贝
```

→ 页缓存：[Ch 15 section-2](../chapter-15-page-cache/notes/section-2-页缓存与address_space.md) · 块层：[Ch 14](../chapter-14-block-devices/)

---

### 二、预读 (Read-Ahead)

**动机：** 顺序读时 **预取** 后续页 — 把 **寻道/旋转** 与进程计算 **重叠**。

| 窗口 | 作用 |
|------|------|
| **当前窗口 (current)** | 进程已请求/正在用的范围 |
| **预读窗口 (ahead)** | **提前** 抓取到页缓存的后续页 |

**自适应：**

- **顺序访问** → 扩大/维持预读  
- **随机访问** → **关闭** 预读，省内存  

> **深潜可选：** `page_cache_async_readahead`、`ondemand_readahead` — 窗口大小动态调整。

HFT：随机读配置文件 **不应** 触发大量预读；顺序读历史 tick 文件 **受益**。

---

### 三、写：`generic_file_write()`

```
write() syscall
    ↓
generic_file_write()
    ↓
用户数据 → 拷贝到页缓存页
    ↓
标 dirty → 立即返回（规范模式）
    ↓
pdflush / 阈值 / fsync → 写回磁盘
```

脏页过多 → **强制回写** — [Ch 15 section-5](../chapter-15-page-cache/notes/section-5-回写脏页与pdflush.md)。

---

← [2. 访问模式](./section-2-文件访问模式.md) · 下一节 [4. mmap](./section-4-内存映射.md)
