//! Item 34: Box::into_raw / from_raw — 模拟「交给 C / 从 C 收回」所有权

struct HeapInt {
    value: i32,
}

/// Rust → 外部：交出堆所有权
fn export_to_c() -> *mut HeapInt {
    Box::into_raw(Box::new(HeapInt { value: 42 }))
}

/// 外部 → Rust：收回并 drop
unsafe fn reclaim_from_c(ptr: *mut HeapInt) {
    let _boxed = Box::from_raw(ptr);
}

fn main() {
    let ptr = export_to_c();
    unsafe {
        assert_eq!((*ptr).value, 42);
        reclaim_from_c(ptr);
    }
    println!("Box ownership transferred and reclaimed safely");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round_trip() {
        let ptr = export_to_c();
        unsafe {
            assert_eq!((*ptr).value, 42);
            reclaim_from_c(ptr);
        }
    }
}
