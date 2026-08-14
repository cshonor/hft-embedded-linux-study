//! `ptr` 底层内存操作 — 绕过 Drop、字节级拷贝。

pub fn write_and_copy() -> ([u8; 4], [u8; 4]) {
    let src = [1u8, 2, 3, 4];
    let mut dst = [0u8; 4];
    let mut overlap = [9u8; 4];

    unsafe {
        // 盲写：目标无需先 Drop 旧值（此处为 plain u8）
        std::ptr::write(dst.as_mut_ptr(), src[0]);
        std::ptr::write(dst.as_mut_ptr().add(1), src[1]);

        // memcpy：不重叠区域
        std::ptr::copy_nonoverlapping(
            src.as_ptr().add(2),
            dst.as_mut_ptr().add(2),
            2,
        );

        // memmove：允许重叠
        std::ptr::copy(overlap.as_ptr(), overlap.as_mut_ptr().add(1), 3);
    }

    (dst, overlap)
}
