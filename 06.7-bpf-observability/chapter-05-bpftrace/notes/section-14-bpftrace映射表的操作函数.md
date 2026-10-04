# 5.14 bpftrace 映射表的操作函数

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.14 节（印刷 p177–182）

## 内容详解

映射表 = BPF 特殊哈希表存储对象（键值对/统计值）。表 5-7：

| 函数 | 描述 |
|------|------|
| `count()` | 计数 |
| `sum(int n)` | 求和 |
| `avg(int n)` | 平均 |
| `min(int n)` / `max(int n)` | 最小/最大值 |
| `stats(int n)` | 次数+平均值+总和 |
| `hist(int n)` | 2 的幂次直方图 |
| `lhist(int n, int min, int max, int step)` | 线性直方图 |
| `delete(@m[key])` | 删除键值对 |
| `print(@m [, top [, div]])` | 打印（可带 top 限制与除数） |
| `clear(@m)` | 删除全部键值对 |
| `zero(@m)` | 全部值置 0 |

**异步函数**：print()、clear()、zero()——内核入队、用户态稍后处理，编程时记住有延迟。

### 5.14.1 count()

```bash
# 通配符 + probe 内置变量按探针计数
t:block:* { @[probe] = count(); }
@[tracepoint:block:block_rq_issue]: 1
@[tracepoint:block:block_bio_queue]: 270 ...

# 周期打印滚动计数
interval:s:1 { print(@); clear(@); }
```

BEGIN 探针可 printf 输出头解释，time() 打时间戳——比 perf stat 可定制性强。

### 5.14.2 sum()、avg()、min()、max()

```bash
t:syscalls:sys_exit_read /args->ret > 0/ { @bytes = sum(args->ret); }
@bytes: 461603
```

过滤器滤掉负值（-errno）后求和才有意义；映射表名（bytes）即输出含义。

### 5.14.3 hist()

- 2 的幂区间直方图；read(2) 返回值示例呈现**多峰**：一峰 ≤0（错误/EOF）、一峰 [1]、一峰 [8,16)；
- 区间表示法：`[` 含等于、`]` 含等于、`(` 严格大于、`)` 严格小于、`..` 无限——`[4,8)` = 4 到 7.99…。

### 5.14.4 lhist()

```bash
t:syscalls:sys_exit_read { @ret = lhist(args->ret, 0, 1000, 100); }
```

- min/max/step 定制线性桶：`(..0) 1011`、`[0,100) 1569`、…、`[1000,..) 51`；
- `(..0)` 行顺便**给出错误计数**；更专业的做法是按错误码统计：`@[-args->ret] = count()` → `@[11]: 57`（EAGAIN）。

### 5.14.5 delete()

`delete(@map[key])`——键可为复合键。双探针计时模式必备（防陈旧时间戳累积）。

### 5.14.6 clear() 和 zero()

- clear 清空全部键值对，zero 只置 0（保留键，滚动速率统计用 zero 更省）；
- **防自动打印技巧**：中间量映射表（如 @start 时间戳表）不想出现在退出输出里，在 END 中 clear：

```awk
END { clear(@start); }
```

### 5.14.7 print()

- `print(@m, top, div)`：top=只打印最高 N 项；div=整数分母（打印时整除）；
- top 示例——最活跃的 5 个 vfs 函数：

```bash
kprobe:vfs* { @[probe] = count(); } END { print(@, 5); clear(@); }
@[kprobe:vfs_read]: 2921
```

- **div 的意义**（纳秒→毫秒的经典坑）：若先 `sum((nsecs-@start[tid])/1000000)`，sum 对整数操作**向下取整**，小于 1ms 的样本全变 0 → 累积误差。正确做法：**sum 纳秒原值，print 时除**：`print(@ms, 0, 1000000)`。

## HFT 关联

- 延迟统计三件套：`hist()`（看分布形态）、`lhist(0,1000,100)`（聚焦某区间）、`stats()`（一行摘要）；
- 滚动每秒速率：`interval:s:1 { print(@); zero(@); }`（保留键避免重建哈希）；
- print 的 div 技巧是所有纳秒→毫秒统计的规范写法。

## 陷阱

- ⚠️ sum()/avg() 先除后存 = 精度损失（整数截断）；**先存原值、打印时除**。
- ⚠️ print/clear/zero 异步——同一动作块里 clear 后立即 print 可能打出旧值，注意顺序与延迟。
- ⚠️ lhist 的 `(..min)` 行天然是"越界/错误"计数器，读数时别漏看。

<details>
<summary>自测题</summary>

1. print(@ms, 0, 1000000) 中 div 参数解决什么问题？
   <details><summary>答案</summary>先对纳秒求和保留精度，打印时才除以 1000000 转毫秒；避免逐样本整数截断累积误差。</details>

2. 滚动每秒计数用 clear 还是 zero？为什么？
   <details><summary>答案</summary>zero（值清零保留键）更高效；clear 删除全部键导致哈希重建。</details>

3. `@[.., 0)` 之类的首行直方图区间代表什么？
   <details><summary>答案</summary>小于区间下限的值（如负返回值/错误），相当于越界计数器。</details>
</details>
