## 6. 共享内存与小结

---

### 一、管道 vs 共享内存

| | **Pipe** | **Shared Memory** |
|---|----------|-------------------|
| 数据 | **字节流** | **结构化 · 随机访问** |
| 拷贝 | **内核/消息队列传递** | **映射后直读写** |
| 同步 | **阻塞 Read/Write** | **需自管 · 易数据竞争** |

---

### 二、共享内存实现思路（概念）

**利用 Ch27 分页：**

```
AllocateFrame(P)
Task A PML4: VA 0x…1000 → PFN P  (Writable|User)
Task B PML4: VA 0x…2000 → PFN P  (Writable|User)
```

**两应用 **不同 VA** · **同一物理帧** — 写 **一方可见另一方**。

| 用途 | 大数组 · struct · **游戏状态** |
|------|-------------------------------|
| 风险 | **Data Race** — 需 **锁/原子**（本书 **提醒** · 未必全实现） |

→ [Ch27 MapFile/CoW](../chapter-27-app-memory/) · [CSAPP Ch12 IPC](../../../01-CSAPP-3rd/)

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **`echo $?`** | **隐式 exit code** |
| **`\|` 管道** | **PipeDescriptor · kPipe** |
| **并发左右** | **流式 · 省内存** |
| **WaitFinish** | **子 Task 同步** |
| **sort · cat 行读** | **管道组合** |
| **共享内存** | **高级 IPC 展望** |

```
Ch29 管道成熟
    ↓
Ch30 额外应用 · 书末
```

---

### 四、后续索引

| Ch29 主题 | 继续读 |
|----------|--------|
| 额外应用 | [chapter-30-extra-apps](../chapter-30-extra-apps/) ⚪ |
| 重定向 | [chapter-28-japanese-redirect](../chapter-28-japanese-redirect/) |
| stdio | [chapter-26-app-write-file](../chapter-26-app-write-file/) |
| TLPI | [06-The-Linux-Programming-Interface pipe/shm](../../../06-The-Linux-Programming-Interface/) |

---

← [5. sort/cat](./section-5-sort-cat优化与终端修复.md) · [Ch 28](../chapter-28-japanese-redirect/) · [Ch 29 导读](../README.md)
