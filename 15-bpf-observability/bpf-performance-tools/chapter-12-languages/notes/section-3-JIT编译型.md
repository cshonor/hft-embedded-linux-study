# 3. JIT 编译型 (Java, Node.js…)

字节码在 **运行时** JIT 为机器码 — 地址 **移动**，符号 **不在 ELF**。

### 挑战

| 问题 | 后果 |
|------|------|
| 方法地址变化 | uprobe 按固定地址会 **失效** |
| 无 ELF 符号 | `profile` 只见 **匿名地址** |
| 频繁重编译 | 旧 map **过期** |

### Java：符号解析

| 组件 | 作用 |
|------|------|
| **`perf-map-agent`** | 注入 JVM，生成 **`/tmp/perf-<PID>.map`** |
| **`jmaps`（Gregg 封装）** | 自动化 map 生成 |

**流程：**

```
启动 Java（见下方 JVM  flags）
    → jmaps / perf-map-agent
    → /tmp/perf-PID.map
    → profile / perf 读 map → 火焰图含 Java 方法名
```

**火焰图前必须 **实时** 生成 map** — JIT 重编译后旧 map 无效。

### Java：调用栈

| JVM 参数 | 必需性 |
|----------|--------|
| **`-XX:+PreserveFramePointer`** | **必须** — 否则 BPF 栈无 Java 帧 |

### Java：USDT 与开销

JVM 内置 USDT（GC、类加载、线程…）：

| 探针类 | 开销 |
|--------|------|
| GC / 线程等 **低频** | 可接受 |
| **`method__entry` 等高频** | 需 `-XX:+ExtendedDTraceProbes` — 书中：**10×+ 惩罚**，**勿生产** |

**HFT：** 共置 **风控/报表 Java** — 用 **JFR/async profiler** 为主；BPF 看 **TCP/ syscall** 层即可。

### Node.js (V8)

| 要点 | 说明 |
|------|------|
| 引擎 | **V8 JIT** — 同 Java 需 **`/tmp/perf-PID.map`** |
| USDT | 需 **`--with-dtrace` 源码编译** Node |
| 场景 | 辅助 Web/脚本服务 — 非 tick 热路径 |

---
