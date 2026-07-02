## 6. 小结与后续索引

---

### 一、本章总结

**vhost-user = 把 virtio 后端搬进 DPDK 用户态 + 共享内存零拷贝：**

| 层次 | 要点 |
|------|------|
| **演进** | Qemu → vhost-net → **vhost-user** |
| **控制** | Unix socket、feature、**mem_table** |
| **数据** | mmap Guest RAM、poll virtqueue |
| **API** | **vhost lib** / **vhost PMD** |
| **实战** | vhost-switch + **VMDQ** + 分核 |

```
Ch11 Virtio 前端
    ↓
Ch12 vhost — 用户态后端 · mem_table · vhost-switch
    ↓
Ch13 NFV（应用篇开篇）— VNF · OPNFV · 商业案例
    ↓
repo 零拷贝 · 组播 · 02-Advanced
```

**结语：** 优异虚拟网络性能需 **前后端双管齐下** — DPDK 打通 Host↔VM **高速通道**。

---

### 二、后续章节索引

| Ch12 主题 | 继续读 |
|----------|--------|
| NFV / VNF 应用篇 | [chapter-13-dpdk-nfv](../chapter-13-dpdk-nfv/) 🟡 |
| Virtio 前端 | [chapter-11-virtio-paravirtualization](../chapter-11-virtio-paravirtualization/) 🟡 |
| I/O 透传 | [chapter-10-x86-io-virtualization](../chapter-10-x86-io-virtualization/) 🟡 |
| VMDQ / 多队列 | [chapter-08-flow-classification-multiqueue](../chapter-08-flow-classification-multiqueue/) 🔴 |
| 大页 / mbuf | [chapter-02-cache-and-memory](../chapter-02-cache-and-memory/) · [Ch6](../chapter-06-pcie-packet-io/) 🔴 |
| 零拷贝 | [chapter-04-零拷贝与用户态旁路.md](../chapter-04-零拷贝与用户态旁路.md) 🔴 |
| 组播落地 | [chapter-05-组播行情接入.md](../chapter-05-组播行情接入.md) 🔴 |
| OVS / XDP | [02-Advanced note-XDP](../../02-Advanced-Book/notes/note-XDP与DPDK对照.md) |
| HFT | [15 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. vhost-switch](./section-5-vhost-switch与实战.md) · [Ch11 Virtio](../chapter-11-virtio-paravirtualization/) · [01-Intro README](../README.md)
