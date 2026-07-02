## ③ 内核调试选项 · Kernel Hacking

`make menuconfig` → **Kernel Hacking**（依赖 **`CONFIG_DEBUG_KERNEL`**）

| 功能示例 | 作用 |
|----------|------|
| **sleep-inside-spinlock 检测** | 在 **原子上下文**（持 spinlock / 关抢占）**非法睡眠** → 抓 **死锁元凶** |

→ **Ch 9–10** 自旋锁 vs mutex 上下文规则

| 现代补充 | **LOCKDEP**、**KASAN**、**KFENCE** — 书中未详述，方向一致 |

---
