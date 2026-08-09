## ③ 映射 · Maps · `idr`

**关联数组** — 将 **唯一键** 映射到 **值**。

Linux **`idr`** 针对的用例：

| 特点 | 说明 |
|------|------|
| 键 | **唯一整数 UID**（常自动分配） |
| 值 | **`void *` 指针**（指向内核对象） |
| 操作 | 添加、删除、查找；添加时可 **自动分配新 UID** |

#### 两步分配（3rd 版书中 API）

| 步骤 | 函数 |
|------|------|
| 1 | **`idr_pre_get()`** — 预分配内存 |
| 2 | **`idr_get_new()`** — 分配 UID 并插入 |

另有查找、销毁整套 API。

> **注：** 新内核中 `idr` 接口有演进，并广泛使用 **`xarray`（xa）** 等结构 — 读书时抓 **「整数 ID → 指针」** 语义即可。

| 典型用途 | 示例 |
|----------|------|
| 小整数 fd 式句柄 | 驱动 minor、内部 handle 表 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** idr 的典型用途是什么？为什么不用哈希表？

<details><summary>答案</summary>

idr 用于分配唯一整数 ID 并快速查找。典型用途：分配 inode 号、设备号、POSIX 定时器 ID。不用哈希表是因为：1) 需要紧凑分配（从 0 开始不跳跃）；2) 需要 O(1) 查找 + O(1) 分配/释放；3) 基数树（radix tree）实现，比哈希表更 cache 友好。

</details>

**Q2.** idr 和现代内核的 xarray 有什么关系？

<details><summary>答案</summary>

xarray 是 idr 的现代替代（Linux 4.20+），API 更简洁、性能更好。`xa_alloc()` 替代 `idr_alloc()`，`xa_load()` 替代 `idr_find()`。idr 仍保留兼容但新代码推荐 xarray。page cache 已从 radix tree 迁移到 xarray。

</details>

</details>
---
