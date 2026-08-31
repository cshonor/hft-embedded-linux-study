# 7.3 BPF 工具（二）：memleak

> 底本：《BPF之巅》第 7 章 内存，7.3.2 节（印刷 p271–274）。

## 原理

BCC 工具：跟踪**内存分配和释放**事件及调用栈，随时间显示**长期未被释放的分配**（outstanding allocations）— 疑似泄漏的代码路径。

```bash
# memleak -p 3126        # 跟踪用户态进程（malloc/calloc/realloc/free）
[09:14:15] Top 10 stacks with outstanding allocations:
    960 bytes in 1 allocations from stack
        xrealloc+0x2a [bash]
        strvec_resize+0x2b [bash]
        maybe_make_export_env+0xa8 [bash]
        execute_command_internal+0x862 [bash]
        ...

# memleak                # 不带 -p：跟踪内核分配（kmem:kmem_cache_alloc 等）
    15384576 bytes in 3756 allocations from stack
        alloc_pages_nodemask+0x209 [kernel]
        handle_pte_fault+0x3bf [kernel]
        do_page_fault+0x250 [kernel]
        ...
```

- 默认每 5 秒输出 Top 10"未释放分配"栈（字节数 + 次数 + 完整栈）
- 用户态：跟踪 malloc/calloc/realloc/free；内核态：kmem 跟踪点
- 输出栈需要帧指针（书例 bash 用 `-fno-omit-frame-pointer` 编译）+ 符号

**重要限定**：memleak 只能列出"分配后很久没释放"的栈 — **无法区分**真泄漏（无引用永不释放）、正常增长、长期存活的合理内存。定性必须**读代码**理解这些路径的意图。

### 配对机制（它凭什么知道"未释放"）

memleak 的核心数据结构是一张**以指针为键的 hash map**：

```
malloc(ptr=0x7f.., size=192, stack=S1)
   │  uprobe/kprobe 触发
   ▼
  map[0x7f..] = { size:192, ts:now, stackid:S1 }     ← 分配记账

free(ptr=0x7f..)
   │
   ▼
  delete map[0x7f..]                                  ← 配对销账

每 5s：遍历 map，按 stackid 聚合输出仍在账上的分配
   （= 分配了、没 free、也没到 -O 年龄线的"悬账"）
```

三个推论：

1. **开销的来源**是每一次 malloc/free 都要走一遍"uprobe 陷入→BPF→map 更新"，与分配频率成正比——不是栈抓取（栈只在记账时抓一次）；
2. **-S 采样**的作用点在最前面：1/RATE 的事件才记账。但 free 也同样被采样——ptr 没记上账的 free 直接跳过，账目两边一致地稀疏，趋势仍对；
3. **map 键空间** = 在飞分配数。百万级在飞分配的对象（比如每连接一个 buffer）会把 map 撑大——这既是内存成本也是遍历成本，-O 过滤缓解输出侧但不缓解 map 侧。

### -S 与 -O 的组合策略

| 场景 | -S | -O | 理由 |
|------|----|----|------|
| 生产环境初筛 | 100+ | — | 先确认"有泄漏迹象"再说 |
| 回测复现（分配率中等） | 10 | 5000 | 滤掉短命缓冲，聚焦长期悬账 |
| 测试环境精确定位 | 1（全量） | 10000 | 慢但账目完整 |

-O 的年龄线怎么定：比"合法缓存的典型存活期"再长一档。行情快照缓存存活秒级，就设 -O 10000（10s）——比它短的悬账大概率是正常周转。

## 命令行

```
memleak [options] [-p PID] [-c COMMAND] [interval [count]]
```

| 选项 | 作用 |
|------|------|
| `-S RATE` | 采样 1/RATE 的分配事件，降开销 |
| `-O OLDER` | 只显示存活超过 OLDER 毫秒的分配 |

## 开销警示

用户态分配每秒可达数百万次（uprobes 逐次陷入内核）→ **性能可降至原来的 1/10 以下**。

定位（原书原话）：memleak 目前是**调试工具**，不是日常性能分析工具。替代方案：
- `-S` 采样降开销
- profile(8) 定时采样粗粒度看 malloc 调用路径
- 等待用户态 uprobes（无内核陷）10–100 倍提速

## HFT 关联

- 策略进程 RSS 缓慢上涨的排查工具：测试环境 `memleak -S 100 -p <pid>`（采样模式）跑回测，看未释放栈是否集中在订单/行情容器路径
- **绝不在生产交易进程上无采样跟踪**（1/10 减速 = 停服）；生产用 faults/brkstack 等低频事件工具
- 定位泄漏三步：memleak 找栈 → 读代码定性（泄漏 or 正常增长）→ 压测复现验证修复

## 常见陷阱

1. **把"未释放分配"直接当泄漏证据** — 长期缓存、启动时一次性分配都合法；必须读代码定性
2. **生产环境全量跟踪** — 每秒百万次 malloc 的进程会掉到 1/10 速度；加 -S 采样或换 profile
3. **栈全是 [unknown]** — 目标没编译帧指针/没符号；CFLAGS=-fno-omit-frame-pointer 重编（或见 13.2.9）
4. **内核态/用户态搞混** — 不带 -p 跟踪的是内核 kmem；用户态进程必须 -p/-c 指定
5. **在飞分配数巨大的进程上 map 爆内存** — 每个未配对分配占一条账（指针+大小+栈 id）；百万级在飞对象时 memleak 自身吃几百 MB 正常，容器里注意配额

<details>
<summary>📝 自测题（点击展开）</summary>

1. **memleak 如何判断"泄漏"？它真的能判断吗？**

   <details>
   <summary>参考答案</summary>

   不能真正判断。它只是记录每个分配的栈并配对 free，超时未配对的分配按栈聚合输出。"未释放"有三种可能：真泄漏（无引用）、正常长期内存（缓存/配置）、暂时增长。区分需要阅读代码路径意图。真正的泄漏判定需要证明"无引用"（GC 语言的可达性分析等）。
   </details>

2. **-S 和 -O 选项分别解决什么问题？**

   <details>
   <summary>参考答案</summary>

   -S RATE 采样：只跟踪 1/RATE 的分配事件，把每秒百万次的开销降到可接受（代价是漏计）；-O OLDER：过滤掉存活短于 OLDER 毫秒的分配，输出聚焦在"长期占用"的分配，减少临时缓冲噪声。
   </details>

</details>
