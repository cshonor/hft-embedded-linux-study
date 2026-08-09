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


### 常见陷阱

1. -F 99 不知道为什么——99 Hz 减少与 OS timer（100/250/1000 Hz）拍频共振
2. 栈回溯不用 fp——dwarf 准但慢体积大，lbr 依赖硬件，fp 最通用但需编译保留
3. 采样时间太短——HFT tail latency 需要长采样才能采到 P99 尖刺的栈

<details>
<summary>自测题（点击展开）</summary>

1. perf record -F 99 为什么用 99 而不是 100？
   <details><summary>答</summary>减与 OS timer（100/250/1000 Hz）拍频共振——避免采样总是落在同一相位</details>
2. 栈回溯的三种方法及 HFT 推荐？
   <details><summary>答</summary>fp（帧指针，推荐需 -fno-omit-frame-pointer）、dwarf（准但慢）、lbr（硬件依赖）</details>
3. 为什么采样时间不能太短？
   <details><summary>答</summary>HFT P99 尖刺是稀有事件——短采样可能采不到，需要足够长才能捕获尾部栈</details>

</details>


---

← [本章导读](../README.md)
