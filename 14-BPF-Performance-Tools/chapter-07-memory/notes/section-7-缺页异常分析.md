# 7. 缺页异常分析

### `faults`

按 **用户态栈** 统计缺页 — 生成 **缺页火焰图** 输入。

```bash
sudo faults-bpfcc -p $(pidof myapp) 30
```

**回答：** **哪些代码路径** 首次触碰内存从而触发物理页分配。

### `ffaults`

按 **文件名** 统计缺页 — 哪类 **共享库/映射文件** 导致大量 fault。

```bash
sudo ffaults-bpfcc 30
```

**场景：** 新部署后冷启动慢 — 是否某 `.so` 被大量 demand paging。

### `hfaults`

按进程统计 **大页 (Huge Page)** 缺页 — 大页是否 **真正生效**。

```bash
sudo hfaults-bpfcc
```

**HFT：** 若配置了 `hugetlbfs` / transparent huge page，用此确认 fault 模式是否符合预期。

---
