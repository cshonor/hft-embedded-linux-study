//! `mem::transmute` — 同尺寸 reinterpret。**极度危险**。
//!
//! 永远不要：`&T` → `&mut T`（永远 UB）。
//! 仅在 layout 已知且类型语义兼容时使用；优先 `transmute_copy` / `from_bits` 等更窄 API。

use std::mem::{size_of, transmute};

pub fn u32_to_bytes_le(x: u32) -> [u8; 4] {
    assert_eq!(size_of::<u32>(), size_of::<[u8; 4]>());
    // 故意用 transmute 演示 Nomicon 语义；生产代码请用 to_le_bytes / to_ne_bytes。
    #[allow(unnecessary_transmutes)]
    let bytes = unsafe { transmute(x) };
    bytes
}

pub fn demo() -> [u8; 4] {
    u32_to_bytes_le(0x0102_0304)
}

// UB 示例（勿运行，仅文档）：
// let r: &i32 = &42;
// let m: &mut i32 = unsafe { transmute(r) }; // ALWAYS UB
