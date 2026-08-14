//! Nomicon ch1：裸指针 `*const T` / `*mut T` 专项 demo。

/// 03 节基础例：`as` 创建 Safe，解引用 unsafe。
pub fn increment_via_raw_ptr() -> i32 {
    let mut x = 42_i32;
    let p: *mut i32 = &mut x as *mut i32; // Safe：仅取地址
    unsafe {
        *p += 1;
    }
    x
}

/// `*const` 只读解引用 vs `*mut` 写入（各用各的指针，避免 const→mut cast）。
pub fn const_vs_mut() -> (i32, i32) {
    let mut n = 10;
    let ro: *const i32 = &n as *const i32;
    let rw: *mut i32 = &mut n as *mut i32;
    let before = unsafe { *ro }; // 通过 *const 读
    unsafe {
        *rw += 5; // 通过 *mut 写
    }
    (before, n)
}

/// 从引用创建裸指针不需要 unsafe；偏移与解引用需要。
pub fn offset_read() -> u8 {
    let bytes: [u8; 4] = [1, 2, 3, 4];
    let base: *const u8 = bytes.as_ptr(); // Safe
    unsafe {
        let second = base.add(1);
        *second
    }
}

/// 整数 cast 成指针：语法 Safe，误用即 UB（此处不解引用无效地址）。
pub fn cast_from_integer() -> usize {
    let p = 0usize as *const u8;
    p as usize // 只打印地址数值，不解引用
}
