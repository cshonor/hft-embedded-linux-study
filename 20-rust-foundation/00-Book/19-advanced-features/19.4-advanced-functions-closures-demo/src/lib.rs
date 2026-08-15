//! 19.4 高级函数与闭包 demo：fn 指针、F: Fn、map 方法项、返回 Box<dyn Fn>

// ── §一 函数指针 fn ─────────────────────────────────────────────────────────

pub fn calc(num: i32, func: fn(i32) -> i32) -> i32 {
    func(num)
}

pub fn double(x: i32) -> i32 {
    x * 2
}

pub fn square(x: i32) -> i32 {
    x * x
}

pub fn add_one(x: i32) -> i32 {
    x + 1
}

pub fn do_twice(f: fn(i32) -> i32, arg: i32) -> i32 {
    f(arg) + f(arg)
}

/// 泛型 F: Fn — 同时接受函数指针与闭包
pub fn run_func<F: Fn(i32) -> i32>(x: i32, f: F) -> i32 {
    f(x)
}

// ── §二 map 方法项 / 构造器 ─────────────────────────────────────────────────

#[derive(Debug, PartialEq, Eq, Clone)]
pub struct Wrapper(u32);

#[derive(Debug, PartialEq, Eq, Clone)]
pub struct Point(pub i32, pub i32);

#[derive(Debug, PartialEq, Eq, Clone)]
pub enum Status {
    Value(u32),
    Stop,
}

// ── §三 返回闭包 ─────────────────────────────────────────────────────────────

pub fn returns_closure() -> Box<dyn Fn(i32) -> i32> {
    Box::new(|x| x + 1)
}

pub fn select_closure(flag: bool) -> Box<dyn Fn(i32) -> i32> {
    if flag {
        Box::new(|x| x * 2)
    } else {
        Box::new(|x| x / 2)
    }
}

pub fn make_adder(x: i32) -> impl Fn(i32) -> i32 {
    move |y| x + y
}

pub fn get_fn() -> Box<dyn Fn()> {
    Box::new(|| println!("  只读闭包"))
}

pub fn get_fnmut() -> Box<dyn FnMut()> {
    let mut cnt = 0;
    Box::new(move || {
        cnt += 1;
        println!("  计数: {cnt}");
    })
}

// ── 整合 demo ───────────────────────────────────────────────────────────────

pub fn demo_fn_pointer() {
    println!("  calc(10, double) = {}", calc(10, double));
    println!("  calc(10, square) = {}", calc(10, square));
    println!("  do_twice(add_one, 5) = {}", do_twice(add_one, 5));
}

pub fn demo_fn_vs_generic() {
    println!("  run_func(5, add_one) = {}", run_func(5, add_one));
    println!("  run_func(5, |x| x - 1) = {}", run_func(5, |x| x - 1));
}

pub fn demo_map_method_and_constructors() {
    let nums = vec![1, 2, 3];
    let s1: Vec<String> = nums.iter().map(|n| n.to_string()).collect();
    let s2: Vec<String> = nums.iter().map(ToString::to_string).collect();
    assert_eq!(s1, s2);
    println!("  map(ToString::to_string): {s2:?}");

    let wrappers: Vec<Wrapper> = (0u32..3).map(Wrapper).collect();
    println!("  map(Wrapper): {wrappers:?}");

    let points: Vec<Point> = nums.iter().map(|&x| Point(x, x * 2)).collect();
    println!("  map(Point): {points:?}");

    let statuses: Vec<Status> = (0u32..5).map(Status::Value).collect();
    println!("  map(Status::Value): {statuses:?}");
}

pub fn demo_return_closure() {
    let f = returns_closure();
    println!("  returns_closure()(10) = {}", f(10));

    let f1 = select_closure(true);
    let f2 = select_closure(false);
    println!("  select_closure(true)(10) = {}", f1(10));
    println!("  select_closure(false)(10) = {}", f2(10));

    let add_two = make_adder(2);
    println!("  make_adder(2)(3) = {}", add_two(3));
}

pub fn demo_fnmut_return() {
    let f = get_fn();
    f();

    let mut fmut = get_fnmut();
    fmut();
    fmut();
}

pub fn demo_all_functions_closures() {
    println!("--- §一 函数指针 fn ---");
    demo_fn_pointer();

    println!("\n--- §一 fn vs F: Fn ---");
    demo_fn_vs_generic();

    println!("\n--- §二 map 方法项/构造器 ---");
    demo_map_method_and_constructors();

    println!("\n--- §三 返回闭包 ---");
    demo_return_closure();

    println!("\n--- §三 FnMut 返回 ---");
    demo_fnmut_return();
}

pub fn demo_compile_error_notes() {
    println!("  【易混 1】fn vs Fn");
    println!("    fn(i32)->i32  = 函数指针类型（具体类型）");
    println!("    Fn(i32)->i32  = trait（闭包/函数指针可实现）");
    println!();
    println!("  【易错 2】闭包传给 fn 参数");
    println!("    fn calc(f: fn(i32)->i32) 不能接 |x| x*2");
    println!("    → 改用 F: Fn(i32)->i32 或 Box<dyn Fn>");
    println!();
    println!("  【易错 3】impl Fn 多分支返回不同闭包");
    println!("    if flag {{ |x| x*2 }} else {{ |x| x/2 }}  // 两种匿名类型");
    println!("    → 须 Box<dyn Fn(i32)->i32>");
    println!();
    println!("  【易错 4】修改捕获却返回 Box<dyn Fn>");
    println!("    → 用 Box<dyn FnMut>");
    println!();
    println!("  【易错 5】Box<dyn Fn> 零开销");
    println!("    → vtable 动态分发，有微小开销");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn calc_double() {
        assert_eq!(calc(10, double), 20);
    }

    #[test]
    fn run_func_accepts_closure() {
        assert_eq!(run_func(5, |x| x - 1), 4);
    }

    #[test]
    fn map_to_string_equivalent() {
        let nums = vec![1, 2];
        let a: Vec<String> = nums.iter().map(|n| n.to_string()).collect();
        let b: Vec<String> = nums.iter().map(ToString::to_string).collect();
        assert_eq!(a, b);
    }

    #[test]
    fn select_closure_branches() {
        assert_eq!(select_closure(true)(10), 20);
        assert_eq!(select_closure(false)(10), 5);
    }

    #[test]
    fn make_adder_impl_fn() {
        let add = make_adder(2);
        assert_eq!(add(3), 5);
    }
}
