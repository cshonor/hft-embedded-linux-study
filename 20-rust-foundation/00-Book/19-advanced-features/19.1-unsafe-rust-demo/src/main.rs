//! 第 19 章：unsafe 超能力示意（勿在生产照搬任意地址等 UB 示例）。

unsafe fn dangerous() {}

/// 与标准库 `split_at_mut` 同思路：在断言成立时，用裸指针构造两段不重叠的可变 slice。
fn split_at_mut_i32(slice: &mut [i32], mid: usize) -> (&mut [i32], &mut [i32]) {
    let len = slice.len();
    let ptr = slice.as_mut_ptr();
    assert!(mid <= len);
    unsafe {
        (
            std::slice::from_raw_parts_mut(ptr, mid),
            std::slice::from_raw_parts_mut(ptr.add(mid), len - mid),
        )
    }
}

extern "C" {
    fn abs(input: i32) -> i32;
}

static HELLO_WORLD: &str = "Hello, world!";

static mut COUNTER: u32 = 0;

fn add_to_count(inc: u32) {
    unsafe {
        COUNTER += inc;
    }
}

unsafe trait UnsafeMarker {}

unsafe impl UnsafeMarker for u32 {}

fn _unsafe_marker_is_implemented_for_u32()
where
    u32: UnsafeMarker,
{
}

#[allow(dead_code)]
union IntOrFloat {
    i: u32,
    f: f32,
}

/// 供 C 等通过符号名链接时使用的示例（实际供 C 调用通常配合 `cdylib` 等配置）。
#[no_mangle]
pub extern "C" fn call_from_c() {
    println!("Just called a Rust function from C (symbol exported).");
}

fn main() {
    println!("--- raw pointers ---");
    let mut num = 5;
    let r1 = &num as *const i32;
    let r2 = &mut num as *mut i32;
    unsafe {
        println!("r1 is: {}", *r1);
        println!("r2 is: {}", *r2);
    }

    println!("--- unsafe fn ---");
    unsafe {
        dangerous();
    }

    println!("--- split_at_mut_i32 ---");
    let mut v = vec![1, 2, 3, 4, 5, 6];
    let r = &mut v[..];
    let (a, b) = split_at_mut_i32(r, 3);
    assert_eq!(a, &mut [1, 2, 3]);
    assert_eq!(b, &mut [4, 5, 6]);

    println!("--- extern C abs ---");
    unsafe {
        println!("Absolute value of -3 according to C: {}", abs(-3));
    }

    println!("--- static ---");
    println!("name is: {HELLO_WORLD}");

    add_to_count(3);
    unsafe {
        // 避免对 `static mut` 形成共享引用（Rust 2024 `static_mut_refs`）。
        println!("COUNTER: {}", std::ptr::addr_of_mut!(COUNTER).read());
    }

    println!("--- union ---");
    let u = IntOrFloat { i: 1 };
    unsafe {
        println!("union as u32: {}", u.i);
    }

    _unsafe_marker_is_implemented_for_u32();
    call_from_c();
}
