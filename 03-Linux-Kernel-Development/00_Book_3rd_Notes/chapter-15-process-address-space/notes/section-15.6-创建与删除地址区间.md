## ⑥ 创建与删除地址区间

#### 创建 · `do_mmap()`

| 路径 | 说明 |
|------|------|
| 用户态 | **`mmap()` / `mmap2()`** syscall |
| 内核 | **`do_mmap()`** 加入地址空间 |

| 优化 | 新区间与 **相邻 VMA 权限相同** → **合并** 成一段 |

#### 删除 · `do_munmap()`

| 用户态 | **`munmap()`** |
|--------|----------------|
| 内核 | **`do_munmap()`** 移除区间 |

→ **Ch 5** syscall · **Ch 16** 文件映射与页缓存

```c
/* 用户态概念 */
void *p = mmap(NULL, size, PROT_READ|PROT_WRITE,
               MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
munmap(p, size);
```

**HFT：** `MAP_SHARED` 跨进程共享行情缓冲 · `MAP_LOCKED`/`mlock` 防换出。

---
