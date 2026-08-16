# 用户态库全景

### 3.1 BCC（Python/Lua/C++）

- eBPF 代码作为**字符串**嵌入 Python，BCC 预处理后交 Clang **运行时编译**（第 5 章 CO-RE 五大痛点的来源）
- BCC 提供了自己的类 C 方言：`BPF_RINGBUF_OUTPUT(output, 1)` 一行同时给内核和用户态定义 ring buffer；`output.ringbuf_output(...)` 这种"对象方法"被展开成 `bpf_ringbuf_output()` helper
- 适合入门和原型；**分发生产工具不推荐**（要带编译器工具链，内存占用大：libbpf 版 opensnoop ~9MB vs Python 版 ~80MB，还有运行时编译的启动延迟）
- BCC 工具现已提供 libbpf 重写版（libbpf-tools），共识是用重写版

### 3.2 C + libbpf

- 本书主线方案；CO-RE + 骨架，无编译器依赖
- 入门起点：**libbpf-bootstrap**；XDP 开发用 **libxdp**（xdp-tools 的一部分，配套 XDP Tutorial 是最好的实战教材之一）
- 坑：用户态 C 代码没有验证器保护，内存错误自己负责

### 3.3 Go（三个库）

| 库 | 实现方式 | CO-RE | 现状 |
|---|---|---|---|
| **gobpf** | iovisor，配合 BCC | 跟随 BCC | 停止积极维护，讨论弃用中——别选 |
| **cilium/ebpf** | 纯 Go 实现（无 CGo） | 有 | 广泛使用（GitHub 约 1 万引用），首选 |
| **libbpfgo** | libbpf 的 CGo 封装 | 有 | Tracee/Parca 在用；CGo 边界有性能顾虑 |

**cilium/ebpf 的 bpf2go 工作流**：

```go
// C 文件开头标记（Go 编译器忽略）：
// +build ignore

// 用户态文件里：
//go:generate go run github.com/cilium/ebpf/cmd/bpf2go -cc $BPF_CLANG -cflags $BPF_CFLAGS bpf <C文件> -- -I../headers
```

- `go generate` 一步完成：编译 eBPF 字节码 + 生成 Go 骨架
- 生成物：`bpf_bpfeb.o`/`bpf_bpfel.o`（大端/小端两套字节码）+ 对应 `.go` 文件（编译期按目标平台选用）——类似 `bpftool gen skeleton` 的 Go 版
- 自动生成的结构（名字从 C 代码派生）：

```go
type bpfMaps struct {
    KprobeMap *ebpf.Map `ebpf:"kprobe_map"`
}
type bpfPrograms struct {
    KprobeExecve *ebpf.Program `ebpf:"kprobe_execve"`
}
type bpfObjects struct { bpfPrograms; bpfMaps }

// 使用：
objs := bpfObjects{}
loadBpfObjects(&objs, nil)                     // 加载全部
defer objs.Close()
kp, _ := link.Kprobe("sys_execve", objs.KprobeExecve, nil)  // 附加
objs.KprobeMap.Lookup(mapKey, &value)          // 读 map
```

**libbpfgo** 特点：channel 接收 ringbuf/perfbuf 事件（`bpfModule.InitRingBuffer("events", eventsChannel, buffSize)`）——Go 语言原生异步特性顺滑接入。

### 3.4 Rust（三个库）

| 库 | 定位 | 备注 |
|---|---|---|
| **libbpf-rs** | libbpf 的 Rust 封装，**内核侧仍写 C** | 官方 libbpf 项目出品 |
| **Redbpf** | libbpf 接口的 Rust crates；Rust→LLVM bitcode→eBPF 多步编译 | foniod 安全监控项目；被 Aya 抢走势头 |
| **Aya** | **纯 Rust、直达 syscall**，不依赖 libbpf/BCC/LLVM；实现与 libbpf 相同的 CO-RE 重定位 | rustc 直接编 eBPF；支持类型最全（trace/XDP/TC/cgroup/LSM）；生态趋势所在（lockc 从 libbpf-rs 迁到 Aya） |

Aya 内核侧代码长这样（对比 C 的 `SEC("xdp/myapp")`）：

```rust
#[xdp(name="myapp")]
pub fn myapp(ctx: XdpContext) -> u32 {
    match unsafe { try_myapp(ctx) } {
        Ok(ret) => ret,
        Err(_) => xdp_action::XDP_ABORTED,
    }
}
unsafe fn try_myapp(ctx: XdpContext) -> Result<u32, u32> {
    info!(&ctx, "received a packet");
    Ok(xdp_action::XDP_PASS)
}

// 用户侧：
let mut bpf = Bpf::load(include_bytes_aligned!("../../target/bpfel-unknown-none/release/myapp"))?;
let program: &mut Xdp = bpf.program_mut("myapp").unwrap().try_into()?;
program.load()?;
program.attach(&opt.iface, XdpFlags::default())
```

- `aya-tool`：生成与内核结构匹配的 Rust 定义（类似 bpftool 生成 vmlinux.h 的作用）
