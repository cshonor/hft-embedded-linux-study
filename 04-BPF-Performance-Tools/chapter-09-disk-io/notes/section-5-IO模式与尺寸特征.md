# 5. I/O 模式与尺寸特征

### `bitesize`

按进程统计 I/O **块大小直方图** — 优化应用 buffer/块对齐。

```bash
sudo bitesize-bpfcc 10
```

| 模式 | 块大小倾向 |
|------|------------|
| 顺序大吞吐 | 大块 |
| 随机小 I/O | 小块 — SSD 仍怕过多 random write |

### `biopattern`

识别 **随机 vs 顺序** 访问百分比。

```bash
sudo biopattern-bpfcc 10
```

**场景：** 数据库/日志是顺序写还是 random read。

### `seeksize`

**寻道距离**（扇区）分布 — 主要为 **HDD** 设计；SSD 上参考意义下降。

```bash
sudo seeksize-bpfcc
```

---
