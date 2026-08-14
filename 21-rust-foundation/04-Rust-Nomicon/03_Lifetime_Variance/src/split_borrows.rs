//! 切片/数组：借用检查器不懂不相交性，须 unsafe 拆分（同 Book `split_at_mut`）。

/// `mid <= len` 已证明时，两段 mutable slice 不重叠。
pub fn split_at_mut<T>(slice: &mut [T], mid: usize) -> (&mut [T], &mut [T]) {
    let len = slice.len();
    assert!(mid <= len);
    let ptr = slice.as_mut_ptr();
    unsafe {
        (
            std::slice::from_raw_parts_mut(ptr, mid),
            std::slice::from_raw_parts_mut(ptr.add(mid), len - mid),
        )
    }
}

pub fn demo() -> (i32, i32) {
    let mut data = [1, 2, 3, 4];
    let (left, right) = split_at_mut(&mut data, 2);
    left[0] += 10;
    right[0] += 100;
    (data[0], data[2])
}
