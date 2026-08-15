# Chapter 05: 虚拟地址空间与 Maple Tree

> 来源：笨叔卷1（地址空间）+ LWN（maple tree）
> 对标：Mel Gorman Ch4（红黑树 → maple tree）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [vm-address-space](notes/01-vm-address-space.md) | 笨叔：进程地址空间布局、VMA 管理、mmap 实现 |
| 2 | [maple-tree](notes/02-maple-tree.md) | LWN：maple tree 替代红黑树、B-tree 变体、RCU 安全遍历 |

## HFT 关联

- **VMA 查找性能**：maple tree 将 VMA 查找从 O(log N) 红黑树优化为 O(log N) 但 cache-friendly 的 B-tree，减少 page fault 延迟
- **mmap 延迟**：HFT 进程 mmap 大量共享内存段时，maple tree 的批量插入比红黑树快 2-3 倍
- **RCU 读路径**：maple tree 支持 RCU 无锁读，查找不需要自旋锁，减少多线程争用
- **NUMA 局部**：maple tree 节点更紧凑，cache line 利用率更高

## 交叉引用

- `05.5-modern-kernel/chapter-03-rcu/`：RCU 基础，maple tree 的读路径
- `06-linux-mm/`：Mel Gorman 红黑树 VMA 管理（已过时）
