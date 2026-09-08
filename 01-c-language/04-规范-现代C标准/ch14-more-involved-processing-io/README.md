# Ch14 · More involved processing and I/O（更复杂的处理与 IO）

> **Level 2 · 相知** · 策略：**⏭️ 跳过**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

文本处理、`scanf`、扩展字符集（Unicode）、二进制文件。HFT 用二进制协议，`scanf`/Unicode 场景少，
跳过即可。

## 一、二进制文件 vs 文本文件

```c
/* 二进制读写（HFT 用） */
FILE *f = fopen("data.bin", "wb");
fwrite(&header, sizeof(header), 1, f);
fwrite(payload, 1, payload_len, f);
fclose(f);

/* 读回 */
FILE *f = fopen("data.bin", "rb");
fread(&header, sizeof(header), 1, f);
fread(payload, 1, payload_len, f);
fclose(f);
```

| 区别 | 文本模式 (`"w"`/`"r"`) | 二进制模式 (`"wb"`/`"rb"`) |
|------|------------------------|---------------------------|
| 换行处理 | Windows: `\n` → `\r\n` | 不转换 |
| 适用 | 配置文件、日志 | 协议数据、二进制格式 |
| HFT | 不用（热路径） | 配置加载阶段 |

> **HFT 注意**：跨平台二进制文件要注意字节序——写文件时用 `htonl`/`htons` 统一为大端。

## 二、`scanf` 的陷阱

```c
/* ⚠ scanf 返回成功匹配的项数，不是输入的值 */
int n = scanf("%d %d", &a, &b);
if (n != 2) { /* 输入不匹配 */ }

/* ⚠ 溢出风险 */
char buf[10];
scanf("%s", buf);     // ❌ 没有长度限制，可能溢出！
scanf("%9s", buf);    // ✅ 限制最多 9 个字符 + '\0'
```

| 问题 | 解决 |
|------|------|
| 缓冲区溢出 | 用 `%9s`（宽度限制）或 `fgets` + `sscanf` |
| 返回值未检查 | 检查返回值，处理不匹配 |
| 输入残留 | `scanf` 不消费换行符，可能影响后续读取 |

> **HFT 建议**：不用 `scanf`，用 `fgets` + `strtok`/`sscanf` 组合更安全。配置文件解析用专门的库（如 `libconfig`、`jansson`）。

## 三、HFT 的 IO 模式

HFT 不用标准库 IO，而是直接操作网络/共享内存：

| 场景 | 标准库 | HFT 实际用 |
|------|--------|-----------|
| 收发报文 | `socket` + `send`/`recv` | DPDK `rte_eth_rx_burst`/`tx_burst`（内核旁路） |
| 日志 | `fprintf` | 写 ring buffer，异步输出 |
| 配置 | `fopen`/`fread` | 启动时读，运行时不用 |
| 共享内存 | — | `mmap` + hugepage（进程间通信） |

## 自测题

<details><summary>1. 为什么 <code>scanf("%s", buf)</code> 是安全隐患？</summary>

`%s` 没有长度限制，输入超过 buf 大小时会溢出缓冲区，覆盖栈上其它数据（经典 buffer overflow 攻击）。
正确写法：`scanf("%9s", buf)`（buf 大小为 10，最多读 9 字符 + '\0'），或用 `fgets(buf, sizeof(buf), stdin)`。
</details>
