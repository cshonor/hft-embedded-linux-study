# no_main 可执行文件模板（Linux · Nightly 参考）

> **不纳入默认 `cargo build`** — 裸机 / OS 开发需按目标平台调整。  
> 编译示例（Linux x86_64）：`rustup run nightly cargo build --target x86_64-unknown-linux-gnu`

---

## 要点清单

| 属性 / 项 | 作用 |
|-----------|------|
| `#![no_std]` | 不链接 std |
| `#![no_main]` | 不生成编译器默认 `main` |
| `#[panic_handler]` | 唯一 panic 处理 |
| `eh_personality` | Nightly lang item，栈展开（若启用） |
| `_start` | Linux 入口，链接器可见 |
| `compiler_builtins` | 补 `memcpy` 等内置符号 |

---

## 参考代码

```rust
#![no_std]
#![no_main]
#![feature(lang_items, rustc_private, panic_info_message)]

use core::arch::asm;
use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[lang = "eh_personality"]
extern "C" fn eh_personality() {}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    // 初始化 .bss、设置栈等（真实内核/裸机须补全）
    let code = demo_add(2, 3);
    exit(code);
}

fn demo_add(a: u32, b: u32) -> u32 {
    a + b
}

fn exit(code: u32) -> ! {
    unsafe {
        asm!(
            "syscall",
            in("rax") 60u64,      // sys_exit
            in("rdi") code as u64,
            options(noreturn)
        );
    }
}
```

---

## Cargo.toml 片段

```toml
[package]
name = "kernel-lite"
version = "0.1.0"
edition = "2018"

[dependencies]
# libc = { version = "0.2", default-features = false }
compiler_builtins = { version = "...", features = ["mem"] }

[profile.dev]
panic = "abort"

[profile.release]
panic = "abort"
```

---

## dev / release panic 策略

| Profile | 可选依赖 | 行为 |
|---------|----------|------|
| **dev** | `panic-semihosting` | 恐慌信息输出到调试主机 |
| **release** | `panic-halt` | 死循环，丢弃诊断 |

通过 `[target.'cfg(...)'.dependencies]` 或 feature 切换。

---

## Windows 注意

入口符号常为 `main` / `WinMain`，与 Linux `_start` 不同；本模板仅作 Nomicon 概念对照。
