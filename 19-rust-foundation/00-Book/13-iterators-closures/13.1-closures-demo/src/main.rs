use std::collections::HashMap;
use std::hash::Hash;
use std::thread;
use std::time::Duration;

fn simulated_expensive_calculation(intensity: u32) -> u32 {
    println!("calculating slowly...");
    thread::sleep(Duration::from_secs(2));
    intensity
}

fn fast_calc(intensity: u32) -> u32 {
    intensity
}

fn generate_workout_without_cache(intensity: u32, random_number: u32, calc: fn(u32) -> u32) {
    if intensity < 25 {
        println!("Today, do {} pushups!", calc(intensity));
        println!("Next, do {} situps!", calc(intensity));
    } else if random_number == 3 {
        println!("Take a break today! Remember to stay hydrated!");
    } else {
        println!("Today, run for {} minutes!", calc(intensity));
    }
}

struct CacherOnce<T>
where
    T: Fn(u32) -> u32,
{
    calculation: T,
    value: Option<u32>,
}

impl<T> CacherOnce<T>
where
    T: Fn(u32) -> u32,
{
    fn new(calculation: T) -> Self {
        Self {
            calculation,
            value: None,
        }
    }

    fn value(&mut self, arg: u32) -> u32 {
        match self.value {
            Some(v) => v,
            None => {
                let v = (self.calculation)(arg);
                self.value = Some(v);
                v
            }
        }
    }
}

struct CacherMap<F, A, R>
where
    F: Fn(&A) -> R,
    A: Eq + Hash,
    R: Clone,
{
    calculation: F,
    values: HashMap<A, R>,
}

impl<F, A, R> CacherMap<F, A, R>
where
    F: Fn(&A) -> R,
    A: Eq + Hash + Clone,
    R: Clone,
{
    fn new(calculation: F) -> Self {
        Self {
            calculation,
            values: HashMap::new(),
        }
    }

    fn value(&mut self, arg: A) -> R {
        if let Some(v) = self.values.get(&arg) {
            return v.clone();
        }
        let v = (self.calculation)(&arg);
        self.values.insert(arg, v.clone());
        v
    }
}

fn generate_workout_with_cache(intensity: u32, random_number: u32, calc: fn(u32) -> u32) {
    let mut expensive = CacherOnce::new(calc);

    if intensity < 25 {
        println!("Today, do {} pushups!", expensive.value(intensity));
        println!("Next, do {} situps!", expensive.value(intensity));
    } else if random_number == 3 {
        println!("Take a break today! Remember to stay hydrated!");
    } else {
        println!("Today, run for {} minutes!", expensive.value(intensity));
    }
}

fn closure_capture_examples() {
    let x = 4;
    let equal_to_x = |z: i32| z == x;
    assert!(equal_to_x(4));

    let mut count = 0;
    let mut inc = || {
        count += 1;
        count
    };
    assert_eq!(1, inc());
    assert_eq!(2, inc());

    let v = vec![1, 2, 3];
    let consume = move || v;
    assert_eq!(consume(), vec![1, 2, 3]);

    let s = String::from("drop me");
    let drop_it = move || drop(s);
    drop_it();

    // move + 只读 → Fn，可多次调用（13.1.4 §二.1）
    let msg = String::from("move readonly");
    let move_readonly = move || println!("  move+read: {msg}");
    move_readonly();
    move_readonly();

    // 闭包形参 &mut，但修改捕获 x → FnMut（13.1.4 §二.2）
    let mut x = 0;
    let mut capture_via_param = |v: &mut i32| {
        x += *v;
    };
    capture_via_param(&mut 2);
    assert_eq!(x, 2);
}

fn type_inference_examples() {
    // 调用推导 — 见 13.1.3
    let add = |x| x + 1;
    assert_eq!(add(5i32), 6);

    // 全标注 — 无需调用即可编译
    let add_annotated = |x: i32| -> i32 { x + 1 };
    assert_eq!(add_annotated(3), 4);

    // let _fail = |x| x + 1; // ❌ E0282 if never called — see 13.1.3
}

fn move_vs_borrow_examples() {
    let s = String::from("hello");
    let borrow = || println!("  borrow: {s}");
    borrow();
    println!("  still ok: {s}");

    let s2 = String::from("world");
    let owned = move || println!("  move: {s2}");
    owned();
    // println!("{s2}"); // ❌ E0382 — ownership moved

    // move + Copy：改闭包内副本，外面不变（13.1.5 §三）
    let mut a = 1;
    let mut f_borrow = || a += 1;
    f_borrow();
    assert_eq!(a, 2);

    let mut b = 1;
    let mut f_move = move || {
        b += 1;
        b
    };
    assert_eq!(f_move(), 2); // 闭包内副本变成 2
    assert_eq!(b, 1);        // 外面仍是 1
}

fn fn_trait_examples() {
    fn take_fn<F: Fn()>(f: F) {
        f();
    }
    fn take_fnmut<F: FnMut()>(mut f: F) {
        f();
    }
    fn take_fnonce<F: FnOnce()>(f: F) {
        f();
    }

    take_fn(|| println!("  take_fn: read-only"));
    take_fnmut(|| println!("  take_fnmut: also ok for read-only"));
    take_fnonce(|| println!("  take_fnonce: widest bound"));

    let s = String::from("a");
    let read_only = || println!("  Fn → FnOnce ok: {s}");
    take_fnonce(read_only);

    let x = 10i32;
    let copy_move = move || println!("  move Copy still Fn: {x}");
    copy_move();
    copy_move();
    println!("  outer x still: {x}");

    // 普通 fn 也 impl Fn
    fn greet() {
        println!("  fn item as Fn");
    }
    take_fn(greet);

    // 降级失败（取消注释会编译报错）：
    // let mut n = 0;
    // let mutating = || { n += 1; };
    // take_fn(mutating); // ❌ FnMut 不能降级到 Fn
}

fn thread_move_example() {
    let s = String::from("hello");
    let handle = thread::spawn(move || {
        println!("thread move: {s}");
    });
    handle.join().unwrap();
}

fn syntax_examples() {
    fn add_fn(x: i32) -> i32 {
        x + 1
    }
    let _v1 = add_fn(5);

    let add1 = |x: i32| -> i32 { x + 1 };
    let add2 = |x: i32| { x + 1 };
    let add3 = |x: i32| x + 1;
    let add4 = |x| x + 1;

    assert_eq!(add1(5), 6);
    assert_eq!(add2(5), 6);
    assert_eq!(add3(5), 6);
    assert_eq!(add4(5), 6);

    let base = 10;
    let add_base = |x| base + x;
    assert_eq!(add_base(5), 15);

    // §五 无参闭包 + 三种捕获
    let num = 100;
    let read_capture = || num + 1;
    assert_eq!(read_capture(), 101);

    let mut counter = 0;
    let mut mut_capture = || {
        counter += 1;
    };
    mut_capture();
    assert_eq!(counter, 1);

    let owned = String::from("move");
    let move_capture = move || owned.len();
    assert_eq!(move_capture(), 4);

    // §五 参数 + 捕获
    let add_param = |n: i32| n + 1;
    assert_eq!(add_param(2), 3);
    assert_eq!(add_param(5), 6);

    let base_only = 10;
    let add_capture = || base_only + 1;
    assert_eq!(add_capture(), 11);

    let base_combo = 10;
    let calc = |n: i32| base_combo + n;
    assert_eq!(calc(3), 13);
    assert_eq!(calc(7), 17);

    let add_multi = |x: i32| -> i32 {
        let tmp = x + 2;
        tmp - 1
    };
    assert_eq!(add_multi(5), 6);
}

fn run_quick() {
    syntax_examples();
    println!("== quick: CacherOnce (fast_calc) ==");
    generate_workout_with_cache(10, 7, fast_calc);

    let mut c = CacherMap::new(|a: &u32| -> u32 { a * 10 });
    assert_eq!(10, c.value(1));
    assert_eq!(20, c.value(2));
    assert_eq!(10, c.value(1));

    closure_capture_examples();
    fn_trait_examples();
    type_inference_examples();
    move_vs_borrow_examples();
    thread_move_example();
    println!("quick: all passed");
}

fn run_full() {
    println!("== Without cache (slow x2) ==");
    generate_workout_without_cache(10, 7, simulated_expensive_calculation);

    println!("\n== With CacherOnce (slow x1) ==");
    generate_workout_with_cache(10, 7, simulated_expensive_calculation);

    let mut c = CacherMap::new(|a: &u32| -> u32 { *a });
    assert_eq!(1, c.value(1));
    assert_eq!(2, c.value(2));

    closure_capture_examples();
    fn_trait_examples();
    type_inference_examples();
    move_vs_borrow_examples();
    thread_move_example();
    println!("\nAll closure examples passed.");
}

fn main() {
    let mode = std::env::args().nth(1).unwrap_or_else(|| "full".to_string());
    match mode.as_str() {
        "quick" => run_quick(),
        "full" | _ => run_full(),
    }
}
