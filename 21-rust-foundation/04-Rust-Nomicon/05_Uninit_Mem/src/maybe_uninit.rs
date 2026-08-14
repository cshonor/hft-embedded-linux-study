//! `MaybeUninit`：Safe 无法部分初始化数组时的标准路径。

use std::mem::MaybeUninit;

/// 逐 slot 写入，再 assume_init_read（须证明已全部初始化）。
pub fn init_array() -> [i32; 3] {
    let mut slots = [
        MaybeUninit::uninit(),
        MaybeUninit::uninit(),
        MaybeUninit::uninit(),
    ];
    for (i, slot) in slots.iter_mut().enumerate() {
        slot.write((i + 1) as i32 * 10);
    }
    unsafe {
        [
            slots[0].assume_init_read(),
            slots[1].assume_init_read(),
            slots[2].assume_init_read(),
        ]
    }
}

/// Vec 经典模式：alloc 容量 → ptr::write 各元素 → set_len（见 maybe_uninit + ptr_ops）。
pub fn vec_from_uninit_slots() -> Vec<i32> {
    let mut v: Vec<i32> = Vec::with_capacity(3);
    unsafe {
        v.as_mut_ptr().write(100);
        v.as_mut_ptr().add(1).write(200);
        v.as_mut_ptr().add(2).write(300);
        v.set_len(3);
    }
    v
}

// 废弃 API（新代码禁用）：
// use std::mem::uninitialized;
// let x: Vec<i32> = unsafe { uninitialized() }; // 用 MaybeUninit 替代
