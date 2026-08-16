# Ch15 · Program failure（程序失败）

> **Level 2 · 相知** · 策略：**🟡 略读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

错误检查策略、清理路径（`goto cleanup` 模式）、`assert` 与 `errno`、退出策略。
作者对"程序失败"的系统化处理与内核 `goto err` 是同一思想。

## 一、错误检查策略

### 返回值 vs errno

C 标准库有两种错误报告机制：

| 机制 | 用法 | 例子 |
|------|------|------|
| 返回值 | 函数返回错误码或特殊值 | `malloc` 返回 NULL、`open` 返回 -1 |
| `errno` | 函数返回值不变，错误码在全局 `errno` | `strtol`、`fopen` |

```c
/* errno 用法 */
#include <errno.h>
#include <string.h>

FILE *f = fopen("config.txt", "r");
if (!f) {
    fprintf(stderr, "open failed: %s\n", strerror(errno));
    // 或用 perror("open failed");
    return -1;
}
```

| errno 规则 | 说明 |
|------------|------|
| 成功时不保证清零 | 必须先检查返回值，确认出错后再看 errno |
| 线程局部 | C11 起 `errno` 是线程局部的（`_Thread_local`） |
| `strerror(errno)` | 转为可读字符串 |
| `perror(prefix)` | 打印 `prefix: error_message` |

### DPDK 错误码约定

```c
/* DPDK: 0 = 成功，负数 = -errno */
int ret = rte_eth_dev_start(port_id);
if (ret < 0) {
    RTE_LOG(ERR, EAL, "dev start failed: %s\n", strerror(-ret));
    // ret = -EINVAL, -ENODEV, -EIO 等
    return ret;
}
```

> **HFT 建议**：项目内统一错误码约定。DPDK 风格（0 成功，负数 = `-errno`）清晰且可移植。

## 二、`goto cleanup` 模式

```c
/* 多步初始化的标准清理模式 */
int init_engine(void)
{
    int ret;
    struct config *cfg = NULL;
    struct ring *rx_ring = NULL;
    struct ring *tx_ring = NULL;
    void *dma_buf = NULL;

    cfg = load_config("engine.conf");
    if (!cfg) {
        ret = -EINVAL;
        goto fail_cfg;
    }

    rx_ring = ring_create(cfg->rx_capacity);
    if (!rx_ring) {
        ret = -ENOMEM;
        goto fail_rx;
    }

    tx_ring = ring_create(cfg->tx_capacity);
    if (!tx_ring) {
        ret = -ENOMEM;
        goto fail_tx;
    }

    dma_buf = malloc(cfg->dma_buf_size);
    if (!dma_buf) {
        ret = -ENOMEM;
        goto fail_dma;
    }

    /* 全部成功 */
    return 0;

/* 清理路径：逆序释放（LIFO 栈式） */
fail_dma:
    ring_destroy(tx_ring);
fail_tx:
    ring_destroy(rx_ring);
fail_rx:
    config_free(cfg);
fail_cfg:
    return ret;
}
```

| 规则 | 说明 |
|------|------|
| 标签命名 | `fail_<step>` 或 `err_<step>`，清晰对应失败点 |
| 逆序释放 | 先分配的最后释放（LIFO） |
| `goto` 只往前跳 | 跳到清理标签，不回跳 |
| 每个标签释放对应资源 | 到 `fail_tx` 时 rx_ring 已分配，需释放 |

> 这正是内核 `goto err` 模式，见 [LKD](../../14-hft-engineering/) 和 [Ch3 控制流](../ch03-everything-about-control/README.md)。

## 三、`assert` 与 `errno`

### assert 的正确用法

```c
/* ✅ 编译期断言（C11/C23） */
static_assert(sizeof(struct msg_hdr) == 16, "wire format mismatch");
static_assert(offsetof(struct msg_hdr, seq) == 4, "seq offset wrong");

/* ✅ 运行时断言（不变量检查） */
assert(ring->head <= ring->capacity);  // 开发阶段检查

/* ❌ assert 里放副作用 */
assert(ring_enqueue(r, item) == 0);    // NDEBUG 后消失！

/* ❌ 用 assert 检查外部输入 */
assert(read_config() == 0);            // 外部输入应该用真正的错误处理
```

| assert 用途 | 例子 |
|-------------|------|
| 不变量检查 | `assert(idx < capacity)` |
| 前置条件 | `assert(ptr != NULL)` |
| 编译期校验 | `static_assert(sizeof(hdr) == 16, "")` |
| 不可达代码 | `assert(0 && "should not reach here")` |

> **assert 的哲学**：assert 检查"程序员犯的错"（不变量、逻辑错误），不检查"外部环境错"（文件不存在、网络断开）。后者用返回值/errno。

### errno 的常见值

| errno | 含义 | 常见来源 |
|-------|------|----------|
| `EINVAL` | 无效参数 | 参数检查失败 |
| `ENOMEM` | 内存不足 | malloc 失败 |
| `ENOSPC` | 空间不足 | 磁盘/缓冲区满 |
| `EAGAIN` | 资源暂不可用 | 非阻塞 IO |
| `EINTR` | 被信号中断 | 系统调用被信号打断 |
| `EBUSY` | 资源忙 | 设备/文件被占用 |
| `ENODEV` | 设备不存在 | 硬件未找到 |

## 四、退出策略

### 正常退出 vs 异常退出

| 方式 | 行为 | 适用场景 |
|------|------|----------|
| `return 0;` (from main) | 调用 atexit handlers → 刷新 stdio → 退出 | 正常关闭 |
| `exit(0)` | 同上，可在任意位置调用 | 库中正常退出 |
| `_exit()` / `_Exit()` | 不刷新 stdio，不调 atexit | fork 后子进程 |
| `abort()` | 发送 SIGABRT → core dump | 不可恢复错误 |
| `[[noreturn]] void die()` | 自定义终止函数 | HFT 致命错误处理 |

```c
/* HFT 致命错误处理 */
[[noreturn]] void die(const char *msg, const char *file, int line) {
    fprintf(stderr, "FATAL at %s:%d: %s\n", file, line, msg);
    /* 写崩溃日志、dump 状态 */
    abort();   // 生成 core dump
}

#define DIE(msg) die(msg, __FILE__, __LINE__)
```

### 信号处理与优雅退出

```c
/* HFT 进程通常注册信号处理器实现优雅退出 */
static volatile sig_atomic_t should_stop = 0;

void signal_handler(int sig) {
    (void)sig;
    should_stop = 1;   // 只设 flag，不做复杂操作（async-signal-safe 限制）
}

int main(void) {
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // kill

    while (!should_stop) {
        /* 主循环 */
    }

    /* 优雅清理 */
    cleanup_resources();
    return 0;
}
```

> 信号处理器的限制详见 [Ch19 控制流变化](../ch19-variations-in-control-flow/README.md)。

## HFT / DPDK 关联

| 概念 | HFT 应用 |
|------|----------|
| `goto cleanup` | DPDK 初始化代码的标准错误处理模式 |
| 错误码约定 | 0 成功，负数 = `-errno`（DPDK 风格） |
| `static_assert` | 编译期校验消息结构布局 |
| 信号处理器 | 优雅退出（设置 flag，主循环检查） |
| `abort()` + core dump | 致命错误时保留现场供调试 |

## 自测题

<details><summary>1. <code>goto cleanup</code> 模式有什么好处？标签为什么要逆序排列？</summary>

好处：正常路径保持在最外层（无嵌套），可读性好；清理路径集中管理。标签逆序排列是因为
资源按 LIFO 释放——先分配的最后释放。到 `fail_tx` 时 `rx_ring` 已分配但 `tx_ring` 未分配，
所以只需释放 `rx_ring`。每个标签只释放"在此标签之前已成功分配"的资源。
</details>

<details><summary>2. assert 应该检查什么？不应该检查什么？</summary>

应该检查：程序员犯的错——不变量（`idx < capacity`）、前置条件（`ptr != NULL`）、
不可达代码（`assert(0 && "unreachable")`）。不应该检查：外部环境错——文件不存在、网络断开、
用户输入无效。外部错应该用返回值/errno 做真正的错误处理，因为 assert 在 release 构建中会消失。
</details>

<details><summary>3. 为什么信号处理器里只设 flag，不做复杂操作？</summary>

信号处理器在信号上下文中运行，只能调用 async-signal-safe 函数。`malloc`、`printf`、
大多数 stdio 函数都不是 async-signal-safe——如果在信号处理器里调用它们，且信号打断了
正在执行这些函数的代码，会导致数据结构损坏或死锁。正确做法：信号处理器只设置 `volatile sig_atomic_t`
flag，主循环检查 flag 后做复杂操作。详见 Ch19。
</details>
