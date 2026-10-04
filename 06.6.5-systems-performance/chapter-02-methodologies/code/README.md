# ch02 code · 方法论工具集

> 本章笔记中的程序类代码块补全为完整可运行文件；说明性伪代码块保留在笔记内。

## 程序

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [instrument_demo.cpp](./instrument_demo.cpp) | 2.5 块3 | P0 分层埋点最小实现：热路径只 `fetch_add(relaxed)`，旁路线程算 ticks/s | g++ 13.3 编译 ✓ 运行 ✓（reporter 每秒输出 ticks/s） |
| [queue_calc.py](./queue_calc.py) | 2.6.4 块3/4/5 | 排队论计算器：M/M/1、M/M/c（Erlang-C）、单位换算 | 运行 ✓：λ=7000/μ=10000 → W=0.333ms、Wq=0.233ms，与笔记一致 |
| [scaling_calc.py](./scaling_calc.py) | 2.7.1 块6/7 | Amdahl 定律 + USL 曲线拟合（含拐点 N* 计算） | 运行 ✓：S(8,f=0.1)=4.71；USL 拟合 α=0.0811/β=0.0043，拐点 14.7 核 |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [tcpdump-pps.sh](./scripts/tcpdump-pps.sh) | 2.5 块4 | tcpdump 交叉验证应用内 counter：pps ≈ (N-1)/(t_N - t_1) | `bash -n` ✓（实跑需 root/CAP_NET_RAW） |

## 依赖

- `queue_calc.py`：纯标准库
- `scaling_calc.py`：USL 部分需 `numpy`/`scipy`（`pip3 install --user numpy scipy`；`--amdahl` 参数可跳过）
- `instrument_demo.cpp`：`g++ -g -O2 -Wall -pthread -o instrument_demo instrument_demo.cpp`

## 顺带修正的笔记错误（2026-10 已改）

- `section-2.7.1` 的 Amdahl 表：f=5% 的 S(8) 应为 **5.9×**（原写 4.7×）、f=10% 应为 **4.7×**（原写 4.3×）——
  由本目录 `scaling_calc.py` 实算验证后修正
