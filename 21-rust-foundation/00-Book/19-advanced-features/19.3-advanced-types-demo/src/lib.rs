//! 19.3 高级类型 demo：newtype、type 别名、!、DST/?Sized

use std::collections::HashMap;
use std::fmt;
use std::mem;

// ── 1. newtype ──────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Millimeters(u32);

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Meters(u32);

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Centimeters(u32);

pub fn takes_mm(x: Millimeters) -> u32 {
    x.0
}

pub struct People(HashMap<i32, String>);

impl People {
    pub fn new() -> Self {
        Self(HashMap::new())
    }

    pub fn add(&mut self, id: i32, name: impl Into<String>) {
        self.0.insert(id, name.into());
    }

    pub fn get_name(&self, id: i32) -> Option<&str> {
        self.0.get(&id).map(|s| s.as_str())
    }
}

// ── 2. type alias ───────────────────────────────────────────────────────────

pub type Kilometers = i32;

pub type Thunk = Box<dyn Fn() + Send + 'static>;

pub type LongResult<T> = Result<T, String>;

pub type ClosureBox = Box<dyn Fn(u32) -> u32>;

pub type IoResult<T> = std::result::Result<T, std::io::Error>;

pub fn takes_thunk(t: Thunk) {
    t();
}

pub fn returns_thunk(msg: &'static str) -> Thunk {
    Box::new(move || println!("  {msg}"))
}

// ── 3. never type ! ─────────────────────────────────────────────────────────

pub fn diverge_with_panic() -> ! {
    panic!("diverging function: never returns")
}

pub fn crash() -> ! {
    panic!("触发 panic，永不返回")
}

pub fn parse_or_continue(inputs: &[&str]) -> u32 {
    let mut last = 0u32;
    for s in inputs {
        let v: u32 = match s.parse::<u32>() {
            Ok(n) => n,
            Err(_) => continue,
        };
        last = v;
    }
    last
}

pub fn match_with_panic_branch(x: Result<u32, &'static str>) -> u32 {
    match x {
        Ok(v) => v,
        Err(e) => {
            let _ = e;
            diverge_with_panic()
        }
    }
}

// ── 4. DST / ?Sized ─────────────────────────────────────────────────────────

pub trait Greeter {
    fn greet(&self) -> String;
}

pub struct Robot;

impl Greeter for Robot {
    fn greet(&self) -> String {
        "beep".to_string()
    }
}

pub fn accepts_unsized<T: ?Sized>(v: &T) -> usize {
    let _ = v;
    mem::size_of::<&T>()
}

pub fn print_content<T: ?Sized>(val: &T)
where
    T: fmt::Display,
{
    println!("  内容：{val}");
}

// ── 整合 demo ───────────────────────────────────────────────────────────────

pub fn demo_newtype() {
    let mm = Millimeters(1200);
    let m = Meters(2);
    let cm = Centimeters(200);
    println!(
        "  takes_mm(Millimeters(1200)) = {}",
        takes_mm(mm)
    );
    println!("  Meters({}) / Centimeters({}) — 不可混算", m.0, cm.0);

    let mut people = People::new();
    people.add(1, String::from("Alice"));
    people.add(2, String::from("Bob"));
    println!("  People.get_name(1) = {:?}", people.get_name(1));
}

pub fn demo_type_alias() {
    let x: i32 = 5;
    let y: Kilometers = 7;
    println!("  i32 + Kilometers = {}", x + y);

    takes_thunk(returns_thunk("Thunk called."));
    let r: LongResult<i32> = Ok(100);
    if let Ok(n) = r {
        println!("  LongResult = {n}");
    }

    let cb: ClosureBox = Box::new(|x| x * 2);
    println!("  ClosureBox(5) = {}", cb(5));
    println!("  IoResult ok = {}", IoResult::<()>::Ok(()).is_ok());
}

pub fn demo_never_type() {
    let last = parse_or_continue(&["10", "nope", "20"]);
    println!("  parse_or_continue last = {last}");
    println!(
        "  match_with_panic_branch(Ok(1)) = {}",
        match_with_panic_branch(Ok(1))
    );

    let choice = true;
    let val = if choice {
        100
    } else {
        crash() // ! coerces to i32
    };
    println!("  if/else with ! branch: val = {val}");
}

pub fn demo_dst_and_sized() {
    let s: &str = "hello";
    println!("  size_of::<&str>() = {}", mem::size_of::<&str>());
    println!(
        "  accepts_unsized::<str> = {}",
        accepts_unsized::<str>(s)
    );

    print_content("Rust DST test");
    print_content(&99);

    let r = Robot;
    let g: &dyn Greeter = &r;
    println!("  dyn Greeter greet = {}", g.greet());
    println!(
        "  size_of::<&dyn Greeter>() = {}",
        mem::size_of::<&dyn Greeter>()
    );
}

pub fn demo_all_advanced_types() {
    println!("--- §二 newtype ---");
    demo_newtype();

    println!("\n--- §三 type 别名 ---");
    demo_type_alias();

    println!("\n--- §四 永不类型 ! ---");
    demo_never_type();

    println!("\n--- §五 DST / ?Sized ---");
    demo_dst_and_sized();
}

pub fn demo_compile_error_notes() {
    println!("  【易错 1】newtype vs type");
    println!("    单位隔离 → struct Meters(u32)；简化书写 → type Kilometers = i32");
    println!();
    println!("  【易错 2】裸 DST");
    println!("    let s: str = ...     → E0277 大小未知");
    println!("    let s: &str = ...    → OK");
    println!();
    println!("  【易错 3】?Sized 但 fn f<T: ?Sized>(t: T)");
    println!("    → DST 不能直接传值；须 fn f<T: ?Sized>(t: &T)");
    println!();
    println!("  【易错 4】let x: !");
    println!("    → ! 无值，仅作发散返回值/分支");
    println!();
    println!("  【易错 5】fn foo<T>(t: T) 接收 &str");
    println!("    → 默认 T: Sized；接受 DST 须 T: ?Sized + &T");
    println!();
    println!("  【易错 6】Vec<str>");
    println!("    → 用 Vec<String> 或 Vec<Box<str>>");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn kilometers_alias_compatible_with_i32() {
        let a: Kilometers = 10;
        let b: i32 = 20;
        assert_eq!(a + b, 30);
    }

    #[test]
    fn people_encapsulation() {
        let mut p = People::new();
        p.add(1, String::from("Alice"));
        assert_eq!(p.get_name(1), Some("Alice"));
    }

    #[test]
    fn never_type_if_branch() {
        let val = if true { 100 } else { panic!("never") };
        assert_eq!(val, 100);
    }

    #[test]
    fn print_content_str_and_i32() {
        print_content("x");
        print_content(&42);
    }

    #[test]
    fn closure_box_alias() {
        let cb: ClosureBox = Box::new(|x| x + 1);
        assert_eq!(cb(5), 6);
    }
}
