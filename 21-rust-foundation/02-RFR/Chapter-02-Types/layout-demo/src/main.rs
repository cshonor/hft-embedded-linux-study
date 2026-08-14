//! RFR Chapter 02 · Layout demo
//! Run: cargo run --manifest-path 02-RFR/Chapter-02-Types/layout-demo/Cargo.toml

use std::mem::{align_of, size_of};
use std::ptr::NonNull;

#[derive(Debug)]
struct TestRust {
    a: u8,
    b: u32,
    c: u8,
}

#[repr(C)]
#[derive(Debug)]
struct TestC {
    a: u8,
    b: u32,
    c: u8,
}

#[repr(packed)]
#[derive(Debug)]
struct TestPacked {
    a: u8,
    b: u32,
    c: u8,
}

#[derive(Debug)]
struct DefaultOrder {
    a: u8,
    b: u32,
    c: u16,
}

#[repr(C)]
#[derive(Debug)]
struct DefaultOrderC {
    a: u8,
    b: u32,
    c: u16,
}

enum Either {
    A(u32),
    B(u64),
}

enum Demo {
    A(u8),
    B(u32),
    C(u16),
}

enum Void {}

macro_rules! show_layout {
    ($label:expr, $ty:ty) => {
        println!("--- {} ---", $label);
        println!("  size_of  = {}", size_of::<$ty>());
        println!("  align_of = {}", align_of::<$ty>());
    };
}

macro_rules! show_offsets {
    ($label:expr, $ty:ty, $($f:ident),+ $(,)?) => {
        println!("--- {} ---", $label);
        println!("  size_of  = {}", size_of::<$ty>());
        println!("  align_of = {}", align_of::<$ty>());
        $( println!("  offset {} = {}", stringify!($f), std::mem::offset_of!($ty, $f)); )+
        println!();
    };
}

fn main() {
    println!("target: {}", std::env::consts::ARCH);
    println!();

    show_offsets!(
        "repr(Rust) · Test { a: u8, b: u32, c: u8 }",
        TestRust,
        a,
        b,
        c
    );
    show_offsets!("repr(C) · Test { a: u8, b: u32, c: u8 }", TestC, a, b, c);
    show_offsets!(
        "repr(packed) · Test { a: u8, b: u32, c: u8 }",
        TestPacked,
        a,
        b,
        c
    );

    show_offsets!(
        "repr(Rust) · DefaultOrder { a: u8, b: u32, c: u16 }",
        DefaultOrder,
        a,
        b,
        c
    );
    show_offsets!(
        "repr(C) · DefaultOrder { a: u8, b: u32, c: u16 }",
        DefaultOrderC,
        a,
        b,
        c
    );

    show_layout!("enum Either { A(u32), B(u64) }", Either);
    show_layout!("enum Demo { A(u8), B(u32), C(u16) }", Demo);
    show_layout!("enum Void {}", Void);

    println!("--- enum payload intuition ---");
    println!("  max payload in Demo: u32 = {} B", size_of::<u32>());
    println!("  size_of::<Demo>()   = {} B (payload + tag + align pad)", size_of::<Demo>());
    println!();

    println!("--- Niche / Option ---");
    println!("  u32                   = {}", size_of::<u32>());
    println!("  Option<u32>           = {}", size_of::<Option<u32>>());
    println!("  bool                  = {}", size_of::<bool>());
    println!("  Option<bool>          = {} (niche: byte values 2..=255)", size_of::<Option<bool>>());
    println!("  char                  = {}", size_of::<char>());
    println!(
        "  Option<char>          = {} (invalid Unicode scalars = niche)",
        size_of::<Option<char>>()
    );
    println!("  u64                   = {}", size_of::<u64>());
    println!("  Option<u64>           = {} (separate monomorph layout)", size_of::<Option<u64>>());
    println!("  String                = {}", size_of::<String>());
    println!(
        "  Option<String>        = {} (payload = String; niche may drop tag)",
        size_of::<Option<String>>()
    );
    println!("  &u32                  = {}", size_of::<&u32>());
    println!("  Option<&u32>          = {}", size_of::<Option<&u32>>());
    println!(
        "  Option<NonNull<u32>>  = {}",
        size_of::<Option<NonNull<u32>>>()
    );
    use std::num::{NonZeroU32, NonZeroU8};
    println!(
        "  Option<NonZeroU32>    = {}",
        size_of::<Option<NonZeroU32>>()
    );
    println!(
        "  Option<Box<u32>>      = {}",
        size_of::<Option<Box<u32>>>()
    );
    println!();

    println!("--- Multi-variant + NonZeroU8 niche ---");
    enum TwoNiche {
        A,
        E(NonZeroU8),
    }
    enum FiveNiche {
        A,
        B,
        C,
        D,
        E(NonZeroU8),
    }
    enum FivePlain {
        A,
        B,
        C,
        D,
        E(u8),
    }
    println!("  Two {{ A, E(NonZeroU8) }}           = {} B (tag+payload in 1 byte)", size_of::<TwoNiche>());
    println!("  Five {{ A..D, E(NonZeroU8) }}       = {} B", size_of::<FiveNiche>());
    println!("  Five {{ A..D, E(u8) }}  (no niche)  = {} B", size_of::<FivePlain>());
    let two_a = TwoNiche::A;
    let two_e5 = TwoNiche::E(NonZeroU8::new(5).unwrap());
    let raw = |v: &TwoNiche| unsafe { *(v as *const _ as *const u8) };
    println!("  TwoNiche::A raw byte = {}, E(5) raw byte = {}", raw(&two_a), raw(&two_e5));
    println!();

    println!("--- DST / wide pointers (x86_64) ---");
    println!("  &u32 (thin)              = {}", size_of::<&u32>());
    println!("  &[u32] (fat)             = {}", size_of::<&[u32]>());
    println!("  &str (fat)               = {}", size_of::<&str>());
    println!("  &dyn Debug (fat)          = {}", size_of::<&dyn std::fmt::Debug>());
    println!("  Vec<u32> handle          = {}", size_of::<Vec<u32>>());
    println!("  (fat = data ptr + metadata: len or vtable)");
    println!();

    trait Animal {
        fn make_sound(&self);
    }
    struct Cat;
    impl Animal for Cat {
        fn make_sound(&self) {
            println!("meow");
        }
    }
    let cat = Cat;
    let animal: &dyn Animal = &cat;
    println!("--- dyn Trait / vtable demo ---");
    println!("  size_of::<&dyn Animal>() = {}", size_of::<&dyn Animal>());
    println!("  size_of_val(animal)      = {} (concrete Cat)", size_of_val(animal));
    println!("  size_of::<Cat>()         = {}", size_of::<Cat>());
    println!();

    // --- raw bytes demo (repr(C) only) ---
    #[repr(C)]
    struct MyStruct {
        a: u8,
        b: u32,
        c: u16,
    }

    println!("--- repr(C) MyStruct + raw bytes ---");
    println!("  size_of  = {}", size_of::<MyStruct>());
    println!("  align_of = {}", align_of::<MyStruct>());
    println!("  a @ {}", std::mem::offset_of!(MyStruct, a));
    println!("  b @ {}", std::mem::offset_of!(MyStruct, b));
    println!("  c @ {}", std::mem::offset_of!(MyStruct, c));

    let s = MyStruct {
        a: 0x12,
        b: 0x5678_abcd,
        c: 0xef90,
    };
    dump_bytes("MyStruct", &s);
}

fn dump_bytes<T>(label: &str, val: &T) {
    let n = size_of::<T>();
    let ptr = val as *const T as *const u8;
    let bytes = unsafe { std::slice::from_raw_parts(ptr, n) };
    println!("  {label} raw ({n} B): {:02x?}", bytes);
    println!("  (padding slots may show stack garbage; use offset_of for layout)");
}
