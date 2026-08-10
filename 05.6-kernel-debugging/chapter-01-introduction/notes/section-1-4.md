# 1.4 生产内核 vs 调试内核

> ⬜ 跳读

## 本节要点

| 内核类型 | 特点 | 用途 |
|---------|------|------|
| **生产内核** | 无调试选项、高性能 | 生产环境运行 |
| **调试内核** | 启用 KASAN/LOCKDEP/DEBUG_INFO | 开发和测试 |
| **混合** | KFENCE 等低开销选项 | 生产环境长期开启 |

## 6.x 可在生产环境开启的低开销调试选项

| 选项 | 开销 | 功能 |
|------|------|------|
| `CONFIG_KFENCE=y` | ~1% | 轻量级内存越界检测 |
| `CONFIG_HARDENED_USERCOPY=y` | 极低 | 用户空间拷贝加固 |
| `CONFIG_RANDOMIZE_BASE=y` | 无 | KASLR 地址随机化 |
| `CONFIG_STACKPROTECTOR_STRONG=y` | 极低 | 栈溢出检测 |

## HFT 关联

HFT 生产环境可以用 KFENCE 替代 KASAN（开销从 ~50% 降到 ~1%），在保证实时性的同时检测内存错误。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 KASAN 不适合生产环境，而 KFENCE 适合？

> KASAN 为每个内存分配添加红区 (redzone) 并在每次访问时检查，开销约 50-100%，不适合生产。KFENCE 采用概率采样（默认每 100KB 采样一次），只对被采样的分配做严格检查，开销约 1%。KFENCE 以概率方式发现内存错误，是开发和生产之间的折中。

</details>
