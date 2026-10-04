# 2.10 USDT（用户态预定义静态跟踪）

> 底本：《BPF之巅》第 2 章技术背景，2.10 节（印刷 p62–66，含 2.10.1–2.10.4）

## 是什么

USDT（user-level Statically Defined Tracing）= 用户空间的跟踪点。与内置日志系统不同：**USDT 依赖外部系统跟踪器唤起**——没有跟踪器 attach 时，探针是 nop，不做任何事。随 DTrace 兴起（Linux 侧最早由 SystemTap 支持；BCC 的 USDT 支持由 Sasha Goldshtein 实现，bpftrace 的由作者与 Matheus Marchini 完成）。许多应用编译时需显式开启：`--enable-dtrace-probes` / `--with-dtrace`。

## 2.10.1 添加 USDT 探针（Folly 实例）

方式一：systemtap-sdt-dev 包的头文件与工具；方式二：自定义头文件（Facebook Folly C++ 库）。

用 Folly 的四步：

```cpp
// 1. 包含头文件
#include "folly/tracing/StaticTracepoint.h"

// 2. 在逻辑位置放探针：provider 分类 + 探针名 + 可选参数
FOLLY_SDT(usdt_sample_lib1, operation_start, operationId,
          request_input.c_str());
```

3. 编译后用 readelf 验证（ELF notes 段 `.note.stapsdt`）：

```bash
$ readelf -n usdt_sample_lib1/libusdt_sample_lib1.so
Displaying notes found in: .note.stapsdt
  Provider: usdt_sample_lib1
  Name:     operation_start
  Location: 0x000000000000febe, Base: 0x0, Semaphore: 0x0
  Arguments: -8@-104(%rbx) -8@%rax
```

4. 可选——**探针信号量**（避免无人观测时白付参数构造成本）：

```cpp
FOLLY_SDT_DEFINE_SEMAPHORE(provider, name);
if (FOLLY_SDT_IS_ENABLED(provider, name)) {
    // 昂贵的参数处理，只在探针被激活后执行
    FOLLY_SDT_WITH_SEMAPHORE(provider, name, arg1, arg2);
}
```

跟踪工具激活探针时设置该信号量；使用信号量保护的探针时通常需要指定 PID。

## 2.10.2 工作原理

编译时探针处放 nop 指令；attach 时内核**用 uprobes 把 nop 动态改为 int3**。readelf 显示的 Location 是段内偏移，须加上运行时加载基址（PIE/ASLR 下基址会变）才是实际地址——书中用 `gdb info proc mappings` 查基址再 `disas base+0x6a2` 演示了 nop → int3 的变化。

## 2.10.3 BPF 与 USDT

- BCC：`USDT.enable_probe()`
- bpftrace：`usdt:` 探针类型

```bash
bpftrace -e 'usdt:/tmp/tick:loop { printf("got: %d\n", arg0); }'
# got: 1 / got: 2 / got: 3 ...
```

## 2.10.4 更多资料

"USDT Probe Support in BPF/BCC"（Sasha Goldshtein）；"USDT Tracing Report"（Dale Hamel）。

## HFT 关联

- HFT 自研交易系统最该用的观测手段：**在关键路径埋 USDT**（收到行情→解析完成→策略判定→下单发出），平时零开销（nop），排障时秒级 attach。
- 信号量机制正好解决"参数构造成本"痛点：正常时段绝不付出格式化/拷贝代价。
- 探针名即业务语义（`order:sent`），比 uprobe 偏移稳定得多，代码重构不破坏观测脚本。

## 陷阱

- 发布版没开 `--enable-dtrace-probes` → 二进制里根本没有 notes 段，attach 时才发现在裸奔。CI 里应断言 readelf -n 能看到关键探针。
- 用了信号量保护的探针必须指定 PID（工具要设置该 PID 的信号量），全系统模式会踩坑。
- ASLR/PIE 使探针地址每次运行不同，手工算地址必错——交给 BCC/bpftrace 处理。

## 自测

<details>
<summary>1. USDT 与应用自带日志系统的本质区别？</summary>

USDT 由外部系统跟踪器唤起；无人 attach 时探针是 nop、完全零成本。日志系统是应用自驱动，开启即付出格式化与 IO 成本。
</details>

<details>
<summary>2. USDT 在 ELF 文件的哪里存元数据？attach 时靠什么改写指令？</summary>

.note.stapsdt（notes 段）；attach 由内核 uprobes 机制把 nop 改为断点指令。
</details>

<details>
<summary>3. 信号量（semaphore）解决什么问题？</summary>

让"昂贵参数构造"只在探针被跟踪器激活后执行；未激活时 if 判断直接短路。
</details>
