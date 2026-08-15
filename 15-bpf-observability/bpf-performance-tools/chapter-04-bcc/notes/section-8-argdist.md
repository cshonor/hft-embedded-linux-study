# 4.8 argdist

> 库本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.8 节。多用途工具之四：**参数/返回值分布统计**

## 内容详解

`argdist(8)`：对函数**参数值或返回值**做频率统计（`-C`）或直方图（`-H`），内核态聚合——回答"参数/返回值的**分布**长什么样？"

### 必选参数：`-C`（频率表）或 `-H`（直方图）

### 书中案例：tcp_select_window 零窗口诊断

```bash
argdist -C 't:tcp:tcp_probe:window'    # （同思路示例）
argdist -H 'r::__tcp_select_window (retval > 0) "ret", retval'
```

对 `tcp_select_window` 返回值做直方图，**大量返回值集中在 0 附近 = 零窗口通告**——对端不收、本端发送受阻的信号。这是"逐值分布"才能给出的洞察（均值/计数都看不出堆积点）。

### probe specifier 语法与特殊变量

```
-C|-H 'probe[(signature)][,label]:expr(retval|argN|@entry(argN))[,(filter)]'
```

| 特殊变量 | 含义 |
|----------|------|
| `retval` | 返回值（kretprobe） |
| `$entry(param)` | **入口时刻**的参数值——在返回探针里引用进入时的参数 |
| `$latency` | （配合 tracepoint exit/entry）入口到出口耗时 |

例：`-H 'r::vfs_read():$latency'` 风格的写法可在返回时计算延迟直方图。

### 其他

- 输出默认每秒刷新（`-i` 调整）；
- `-h` 查看完整用法；
- 直方图为 2 的幂次分桶（`print_log2_hist` 同款）。

## HFT 关联

- 四大工具中最被低估的一个：**任何"参数分布异常"问题都归它**——报单大小分布、读返回值分布、锁持有时间分布；
- 例：`argdist -H 'r::my_match (retval>0)'` 看撮合函数返回值分布是否出现异常长尾。

## 陷阱

- ⚠️ `-C` 和 `-H` 必选其一，否则什么也不做；
- ⚠️ 字符串参数需 `$entry(param)` 语义注意：入口探针记录、出口探针读取，直接在 `r::` 里取 `argN` 拿到的是**出口时刻寄存器值**（已不可靠）；
- ⚠️ 与 trace 的分工：要"分布"用 argdist（内核态聚合、便宜），要"每一行"才用 trace。

<details>
<summary>自测题</summary>

1. `-C` 和 `-H` 的区别？
   <details><summary>答案</summary>`-C` 输出离散值频率表；`-H` 输出 2 的幂次直方图（连续值分布）。</details>

2. 在返回探针中引用函数入口参数用什么语法？
   <details><summary>答案</summary>`$entry(param)`。</details>

3. tcp_select_window 返回值直方图集中在 0 附近说明什么？
   <details><summary>答案</summary>零窗口通告——接收方不收数据，发送被阻塞。</details>
</details>
