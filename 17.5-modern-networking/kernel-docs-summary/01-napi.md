# 01 — Documentation/networking/napi.rst

> **对应 Rosen:** Ch1（NAPI 基础）/ Ch14（高级主题）
> **内核源码路径:** `Documentation/networking/napi.rst`

## 文档概述

内核 NAPI 官方文档，描述现代 NAPI 的完整工作流程和驱动 API。

## 核心内容

### NAPI 实例生命周期

```
napi_enable() → POLL 状态 → napi_schedule() → 轮询 → napi_complete()
     ↑                    ↑                         |
     └────────────────────┴─────────────────────────┘
```

### 关键 API

| API | 作用 |
|-----|------|
| `netif_napi_add()` | 注册 NAPI 实例（驱动初始化时） |
| `napi_enable()` | 启用 NAPI |
| `napi_schedule()` | 请求调度 NAPI 轮询（中断处理中调用） |
| `napi_poll()` | 驱动提供的轮询回调 |
| `napi_complete_done()` | 轮询完成，重新开中断 |
| `napi_disable()` | 禁用 NAPI |

### threaded NAPI（5.11+）

```
# 启用
echo 1 > /sys/class/net/eth0/threaded

# NAPI 线程出现在 ps
ps aux | grep napi
# napi/eth0-3   ...
```

### GRO 与 NAPI 的关系

- `napi_gro_receive()`：驱动收包后通过 NAPI 传入 GRO
- GRO 在 NAPI 轮询期间合并包
- `napi_gro_flush()`：NAPI 轮询结束时刷新未合并的包

## HFT 要点

- SO_BUSY_POLL 需要驱动支持 NAPI ID（`napi_id`）
- `ethtool -C` 调节 NAPI 的中断合并参数（rx-usecs / rx-frames）
- threaded NAPI 绑定 CPU：`taskset -c <core> $(pgrep napi/eth0)`

## 与 Rosen 3.x 的差异

- Rosen 描述的 NAPI 是 2.6 时代基础版本
- 现代 NAPI 新增：threaded mode、busy polling、GRO 集成、page_pool 集成
