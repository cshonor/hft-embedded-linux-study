# 4. 解释型语言 (Bash, Python, Ruby…)

**业务代码不直接变成机器码** — CPU 上跑的是 **解释器** 的 C 函数。

### 追踪策略

| 层级 | 做法 |
|------|------|
| **错误** | `uprobe` 追 Python 用户函数名（不存在于 ELF） |
| **可行** | uprobe 追 **`python`/`bash` 内部 C 函数** |
| **最佳** | 解释器源码预埋 **USDT** |

### Bash 示例（书中思路）

追踪 bash 内部如 **`execute_function` / `execute_builtin`** — 得脚本层函数名与延迟。

| 坑 | 说明 |
|----|------|
| 系统 `/bin/bash` 常 **strip** | uprobe 挂内部符号 **失败** |
| 对策 | 带符号 bash 构建，或 USDT |

→ 安全向：`bashreadline` [Ch 11](../../chapter-11-security/)

### Python / Ruby

| 手段 | 说明 |
|------|------|
| uprobe CPython 内部 | 可行但 **脆弱**（版本绑定） |
| **`python -m pyperf` / `py-spy`** | 往往比 BPF 更省心 |
| USDT | 若发行版提供（少见） |

**HFT：** 研究脚本/运维自动化 — 非策略热路径；用 **专用 profiler** 优先。

---
