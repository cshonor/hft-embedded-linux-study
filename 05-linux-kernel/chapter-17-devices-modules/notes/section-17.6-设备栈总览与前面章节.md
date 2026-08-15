## 设备栈总览（与前面章节）

```
应用
  ├─ open("/dev/sdX")     ──► 块层（Ch 14）──► 驱动模块
  ├─ open("/dev/uio")     ──► 字符驱动 / UIO
  └─ socket()             ──► 网络子系统（非 /dev）
         ▲
    sysfs / uevent 暴露拓扑与配置
```



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 从用户态 write() 到网卡发包经过哪些层？

<details><summary>答案</summary>

write(socket, data, len) → VFS（socket file）→ sock_write_iter → 协议栈（TCP/UDP）→ ip_push_pending_frames → 网卡驱动 ndo_start_xmit → DMA → 网卡硬件发包。HFT 优化路径：AF_XDP 绕过协议栈直接从用户态到网卡驱动；DPDK 完全在用户态驱动网卡，不经内核。

</details>

</details>
---
