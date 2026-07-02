## 13.9 `perf record` — 剖析采样

### 工作原理

```
定时/事件触发 → 采当前 PC + 栈（若 -g）
    → 写入 perf.data（含符号表索引）
```

| 选项 | 含义 |
|------|------|
| `-F 99` | 99 Hz 频率采样 |
| `-c N -e cycles` | 每 N 周期采一次 |
| `-g` | **调用栈**（call graph） |
| `--call-graph fp` | 帧指针 unwinding（推荐，需 -fno-omit-frame-pointer） |
| `--call-graph dwarf` | debuginfo 栈 — 准但慢、体积大 |
| `-p PID` | 单进程 |
| `-a` | 全系统 |
| `-e EVENT` | 按事件采（如 page-faults） |
| `-- sleep N` | 采 N 秒 |

```bash
perf record -F 99 -g --call-graph fp -p $(pidof strategy) -- sleep 30
# 或全系统 crisis
perf record -F 99 -g -a -- sleep 10
```

### Stack Walking（栈回溯）配置

| 方法 | 要求 | HFT 推荐 |
|------|------|----------|
| **fp（帧指针）** | `-fno-omit-frame-pointer` | **Release 保留 fp** |
| **dwarf** | `-g` debuginfo | 调试构建 |
| **lbr** | 硬件 Last Branch Record | 部分 CPU |

**Ch 5 Gotchas 落地：**

- `[unknown]` → 装 debuginfo / 勿 strip
- 栈浅/断层 → 开 fp；减 `-O3` inline 或 dwarf

---


---

← [本章导读](../README.md)
