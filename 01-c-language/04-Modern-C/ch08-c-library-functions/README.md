# Ch8 · C library functions（C 标准库函数）

> **Level 1 · 相识** · 策略：**⏭️ 跳过**（当字典查）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

`math.h`、`stdio.h`、字符串处理、时间、运行环境、断言。K&R Ch7/附录B 和 CSAPP 已覆盖；
本章只记录 **HFT 场景的关键注意点**和 **C23 新增库函数**。

## 一、字符串处理

### `strtol` 家族 vs `atoi`

```c
/* ❌ atoi：不能检测错误，溢出行为未定义 */
int val = atoi(str);     // "abc" → 0，你不知道是解析失败还是真的是0

/* ✅ strtol：可检测错误、可指定基数 */
char *endptr;
errno = 0;
long val = strtol(str, &endptr, 10);

if (errno == ERANGE) { /* 溢出 */ }
if (endptr == str)   { /* 没有数字被解析 */ }
if (*endptr != '\0') { /* 有额外字符 */ }
```

| 函数 | 类型 | 安全性 | HFT 用途 |
|------|------|--------|----------|
| `atoi` | int | ❌（无错误检测） | 不要用 |
| `atol` | long | ❌ | 不要用 |
| `strtol` | long | ✅ | 解析十进制配置 |
| `strtoul` | unsigned long | ✅ | 解析无符号值 |
| `strtoll` | long long | ✅ | 64 位值 |
| `strtod` | double | ✅ | 浮点配置 |

### `snprintf` — 安全字符串格式化

```c
char buf[64];
snprintf(buf, sizeof(buf), "ORDER id=%u price=%.2f", id, price);
// 不会溢出 buf；返回值是"本应写入的总长度"（不含\0）
```

> **HFT 红线**：永远不用 `sprintf`（无长度限制，缓冲区溢出）；`strcpy`/`strcat` 同理不用，改用 `strncpy`/`snprintf`/`strlcpy`。

### C23 新增：`memset_explicit`

```c
/* C23：不会被优化器删除的 memset（用于清除敏感数据） */
memset_explicit(key, 0, sizeof(key));   // 密钥清零，编译器不会"优化掉"

/* 传统 memset 可能被编译器删除（因为后面没读 key，写零是"死存储"） */
memset(key, 0, sizeof(key));            // ⚠ 编译器可能优化掉！
```

| 场景 | 函数 |
|------|------|
| 普通清零 | `memset` |
| 敏感数据清零（密钥、密码） | `memset_explicit`（C23）或 `explicit_bzero`（GNU） |

## 二、`<math.h>`

| 函数 | 说明 | HFT 注意 |
|------|------|----------|
| `sqrt`/`sqrtf` | 平方根 | 浮点精度：`sqrtf` 更快但精度低 |
| `pow` | 幂 | 热路径避免：比乘法慢 50-100x |
| `floor`/`ceil`/`round` | 取整 | `round` 返回 double，`lround` 返回 long |
| `fabs` | 绝对值 | 整数用 `abs`/`labs`/`llabs` |

> HFT 热路径通常避免浮点运算；如果必须用，确保不是控制路径（分支预测失败代价大）。

## 三、`<stdio.h>` IO

| 函数 | 用途 | HFT 注意 |
|------|------|----------|
| `printf` | 格式化输出 | 热路径禁用（系统调用 + 格式化开销） |
| `fprintf` | 输出到文件 | 日志用 |
| `snprintf` | 安全格式化 | 配置/日志组装用 |
| `fread`/`fwrite` | 二进制读写 | 配置文件加载 |
| `fopen`/`fclose` | 文件操作 | 初始化阶段用 |

```c
/* HFT 日志模式：热路径不直接 printf，而是写入 ring buffer，由日志线程异步输出 */
struct log_entry *e = ring_alloc(log_ring);
if (e) {
    e->level = LOG_INFO;
    e->ts = rte_rdtsc();
    snprintf(e->msg, sizeof(e->msg), "order %u filled", order_id);
    ring_enqueue(log_ring, e);
}
```

## 四、时间函数

| 函数 | 精度 | 说明 |
|------|------|------|
| `time()` | 秒 | 太粗，HFT 不用 |
| `clock()` | 进程CPU时间 | 不是墙上时间，不用 |
| `clock_gettime(CLOCK_MONOTONIC, &ts)` | 纳秒 | **HFT 标准方式**（用户态） |
| `timespec_get(&ts, TIME_UTC)` | 纳秒 | C11 标准（不如 clock_gettime 通用） |

```c
/* HFT 测量时间：clock_gettime + CLOCK_MONOTONIC */
struct timespec ts;
clock_gettime(CLOCK_MONOTONIC, &ts);
uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;

/* DPDK 用 rte_rdtsc()（TSC 寄存器，更快但需校准频率） */
uint64_t tsc = rte_rdtsc();
double us = (double)(tsc - start_tsc) / rte_get_tsc_hz() * 1e6;
```

## 五、`<assert.h>`

```c
/* 运行时断言 */
assert(ptr != NULL);   // NDEBUG 定义时编译消失

/* C11 静态断言 */
_Static_assert(sizeof(struct msg_hdr) == 16, "wire format mismatch");

/* C23：static_assert 是关键字（不再需要 _Static_assert） */
static_assert(sizeof(struct msg_hdr) == 16, "wire format mismatch");
```

| 要点 | 说明 |
|------|------|
| `assert` | 运行时检查；`NDEBUG` 定义后消失——**不要在 assert 里放副作用** |
| `_Static_assert` / `static_assert` | 编译期检查，不生成代码 |
| HFT 用法 | `_Static_assert` 校验消息结构布局（线协议格式不能错） |

```c
/* ❌ assert 里有副作用 */
assert(ring_enqueue(r, item) == 0);   // NDEBUG 后 enqueue 消失！

/* ✅ 先执行再断言 */
int ret = ring_enqueue(r, item);
assert(ret == 0);
```

## 六、C23 新增标准库亮点

| 头文件 | 内容 | HFT 相关度 |
|--------|------|-----------|
| `<stdbit.h>` | popcount、前导零、旋转 | ⭐⭐ 哈希/位操作 |
| `<stdckdint.h>` | 整数溢出检测 | ⭐⭐⭐ 订单数量计算 |
| `memset_explicit` | 安全清零 | ⭐⭐ 密钥处理 |
| `memccpy` | 复制到指定字节 | ⭐ 字符串处理 |

### `<stdckdint.h>` — 编译器辅助整数溢出检测（C23）

```c
#include <stdckdint.h>

uint32_t a = 0xFFFFFFFF, b = 1, result;
if (ckd_add(&result, a, b)) {
    // 溢出了！result 包含截断结果
    return ERROR_OVERFLOW;
}
// result 安全使用
```

> HFT 场景：订单数量累加、价格计算——溢出是致命 bug。以前用 `__builtin_add_overflow`，C23 后用标准接口。

## HFT / DPDK 关联

| 场景 | 标准库函数 | HFT 实践 |
|------|-----------|----------|
| 字符串解析 | `strtol`（不用 `atoi`） | 配置文件/命令行解析 |
| 安全格式化 | `snprintf`（不用 `sprintf`） | 日志/消息组装 |
| 时间测量 | `clock_gettime` | 性能分析（DPDK 用 `rte_rdtsc`） |
| 编译期校验 | `static_assert` | 消息结构布局保证 |
| 溢出检测 | `ckd_add`/`ckd_mul`（C23） | 数量/价格计算 |
| 敏感清零 | `memset_explicit`（C23） | 密钥清除 |
| 热路径 IO | 不用标准库 | 写 ring buffer，异步输出 |

## 自测题

<details><summary>1. 为什么 <code>atoi("abc")</code> 返回 0 是个问题？</summary>

`atoi` 无法区分"解析失败"和"值真的是 0"——两种情况都返回 0。
`strtol` 通过 `endptr` 参数告诉你解析到哪了：如果 `endptr == str`，说明没有数字被解析；
如果 `*endptr != '\0'`，说明有额外字符。还能通过 `errno == ERANGE` 检测溢出。
HFT 代码中解析配置/协议字段时必须用 `strtol`。
</details>

<details><summary>2. 为什么 <code>memset(key, 0, sizeof(key))</code> 可能被编译器删除？</summary>

如果编译器分析到 `key` 在 memset 之后没有被读取，它认为写零是"死存储"（dead store），
可以安全删除以优化。但清除密钥/密码正是为了安全——数据必须被清零。
C23 的 `memset_explicit` 告诉编译器"这次写必须执行"，不会被优化掉。
C23 之前用 `explicit_bzero`（GNU）或内联汇编 `volatile` 写。
</details>

<details><summary>3. <code>assert(ring_enqueue(r, item) == 0)</code> 有什么问题？</summary>

`assert` 在 `NDEBUG` 定义后（release 构建）会被预处理掉——`ring_enqueue` 调用完全消失！
这把功能逻辑放在断言里是严重 bug。正确做法：先执行 `int ret = ring_enqueue(r, item);`
再 `assert(ret == 0);`。规则：assert 里不能有副作用。
</details>
