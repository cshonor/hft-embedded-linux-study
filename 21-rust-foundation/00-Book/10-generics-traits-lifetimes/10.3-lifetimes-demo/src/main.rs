// 10.3 生命周期与引用有效性 - 示例

use std::fmt::Display;

fn two_str<'a, 'b>(x: &'a str, y: &'b str) {
    println!("{} | {}", x, y);
}

fn longest<'a>(x: &'a str, y: &'a str) -> &'a str {
    if x.len() > y.len() {
        x
    } else {
        y
    }
}

fn get_first<'a>(s: &'a str) -> &'a str {
    let end = s
        .char_indices()
        .nth(1)
        .map(|(i, _)| i)
        .unwrap_or(s.len());
    &s[0..end]
}

fn borrow_num<'a>(data: &'a i32) -> &'a i32 {
    data
}

// 笔记 §7.3：返回入参引用，'a 绑定数据源寿命 → 防悬垂
fn get_ref<'a>(data: &'a i32) -> &'a i32 {
    data
}

// 笔记 §5.8：无返回值 + 内层一长一短 → ✅；有返回值且跨红线才 ❌
#[allow(dead_code)]
fn two_ref<'a>(a: &'a i32, _b: &'a i32) -> &'a i32 {
    a
}

fn calc<'a>(a: &'a i32, b: &'a i32) {
    println!("{} {}", a, b);
}

// 笔记 §5.8：use_both 无返回值，内层一长一短 — 编译正常
fn use_both<'a>(a: &'a i32, b: &'a i32) {
    println!("use_both: {} {}", a, b);
}

// 笔记导读 §2 / §7.5 场景4.1：结构体持引用须标注 'a
struct Holder<'a> {
    val: &'a i32,
}

#[allow(dead_code)]
struct Wrapper<'a> {
    val: &'a i32,
}

struct StrWrapper<'a>(&'a str);

fn make_wrapper(s: &str) -> StrWrapper<'_> {
    StrWrapper(s)
}

fn print_underscore(s: &'_ str) {
    println!("  print_underscore: {s}");
}

#[allow(dead_code)]
fn print_plain(s: &str) {
    println!("  print_plain: {s}");
}

fn cmp<'a>(x: &'a i32, y: &'a i32) -> &'a i32 {
    if *x > *y { x } else { y }
}

// 空函数体：无返回值，内层一长一短调用合法（§5.8）
#[allow(dead_code)]
fn demo<'a>(_x: &'a i32, _y: &'a i32) {}

fn get_data(data: &i32) -> &i32 {
    data
}

#[allow(dead_code)]
fn same(a: &i32, b: &i32) {}

#[allow(dead_code)]
fn same_group_elision<'a>(a: &'a i32, b: &'a i32) {}

fn print_both<'a>(x: &'a i32, y: &'a i32) {
    println!("{} {}", x, y);
}

// 笔记 §5.8：无返回值内层调用 ✅；有返回值且 res 逃出内层才 ❌
#[allow(dead_code)]
fn print_two<'a>(x: &'a i32, y: &'a i32) {
    println!("{} {}", x, y);
}

fn take_same<'a>(a: &'a i32, b: &'a i32) {
    println!("{} {}", a, b);
}

fn two_group<'a, 'b>(x: &'a i32, y: &'b i32) {
    println!("{} {}", x, y);
}

fn triple<'a, 'b, 'c>(x: &'a i32, y: &'b i32, z: &'c i32) {
    println!("{} {} {}", x, y, z);
}

fn pick_one<'a, 'b>(x: &'a i32, _y: &'b i32) -> &'a i32 {
    x
}

fn return_two<'a, 'b>(x: &'a i32, y: &'b i32) -> (&'a i32, &'b i32) {
    (x, y)
}

fn pick_any<'a>(x: &'a i32, y: &'a i32) -> &'a i32 {
    if *x > *y { x } else { y }
}

fn demo1<'a>(x: &'a i32, y: &'a i32) {
    println!("demo1: {} {}", x, y);
}

fn demo2<'a>(p: &'a i32) {
    println!("demo2: {}", p);
}

fn get_short<'a>(x: &'a i32, _y: &'a i32) -> &'a i32 {
    x
}

fn return_ref<'a>(p: &'a i32) -> &'a i32 {
    p
}

fn get_static_str() -> &'static str {
    let s = "常驻文本";
    s
}

// fn return_local() -> &i32 {
//     let num = 66;
//     &num
// }

// fn demo() -> &i32 {
//     let x = 10;
//     &x
// }

// fn bad_str<'a>() -> &'a str {
//     let s = String::from("hello");
//     &s
// }

fn longest_with_an_announcement<'a, T>(x: &'a str, y: &'a str, ann: T) -> &'a str
where
    T: Display,
{
    println!("Announcement! {}", ann);
    if x.len() > y.len() {
        x
    } else {
        y
    }
}

struct ImportantExcerpt<'a> {
    part: &'a str,
}

impl<'a> ImportantExcerpt<'a> {
    fn level(&self) -> i32 {
        3
    }

    fn announce_and_return_part(&self, announcement: &str) -> &str {
        println!("Attention please: {}", announcement);
        self.part
    }
}

fn main() {
    println!("=== 0) 变量生命周期 vs 'a 标注 ===");
    let num = 10;
    let r = borrow_num(&num);
    println!("borrow<'a>: r = {}", r);

    calc(&num, &num); // 导读：多入参共用 'a，纯入参约束
    let holder = Holder { val: &num };
    println!("Holder {{ val: {} }}", holder.val);
    println!("get_data 省略 'a: {}", get_data(&num)); // §10 省略规则

    println!("\n=== 0.55) 匿名生命周期 '_ ===");
    let text = String::from("hello '_");
    let w = make_wrapper(&text);
    print_underscore(w.0);
    println!("  StrWrapper 与 text 同寿，text 仍可用: {text}");
    let longer = longest("short", "longer string");
    println!("  longest 须命名 'a: {longer}");
    // fn bad(x: &'_ str, y: &'_ str) -> &'_ str { x } // ❌ 两个 '_ 不绑定

    println!("\n=== 0.5) &'static str：变量消失，数据不 drop ===");
    {
        let s = "块内字面量";
        println!("块内 s = {}", s);
    }
    println!("返回 &'static str: {}", get_static_str());

    let s: &'static str = "abc";
    println!("s: &'static str → 只读段 \"{}\" 地址 {:p}", s, s);

    println!("\n=== 0.7) 悬垂引用与生命周期防护 ===");
    let val = 99;
    let r2 = get_ref(&val);
    println!("get_ref(&val): {}（返回引用绑定 val 寿命，不会悬垂）", r2);

    // 跨块悬垂反例（编译报错）：
    // let r: &i32;
    // { let x = 10; r = &x; }
    // println!("{}", r);

    // 合法方式1：引用与数据同块
    {
        let x = 10;
        let r = &x;
        println!("同块借用: {}", r);
    }

    // 合法方式2：数据提升到外层
    let x = 10;
    let r: &i32;
    {
        r = &x;
    }
    println!("外层变量 + 跨块引用: {}", r);

    println!("\n=== 0.75) 常见悬垂场景（反例见笔记 §7.5，均编译报错）===");
    // 场景1 return_local() — 返回局部引用
    // 场景3.1 let mut val=10; let r=&val; val=20;
    // 场景3.2 let r = &(10 + 20);
    // 场景4.1 Wrapper { val: &x } 且 x 在内层块
    // 场景4.2 let r=&v[0]; v.clear();
    // 场景6   let r=&s1; let s2=s1;
    println!("合法：数据寿命 ≥ 引用寿命 → 无悬垂");

    // two_ref(&outer, &inner) 见 0.8 内层块注释

    println!("\n=== 0.8) 最晚可用时间 & 'a 分组 ===");
    let v1 = 10;
    let v2 = 20; // 同块 → &v1、&v2 最晚可用时间都是 main 末尾
    println!("cmp: {}", cmp(&v1, &v2));

    let num = 99;
    take_same(&num, &num); // 同一变量，只是特例

    let outer = 100;
    {
        let inner = 200;
        two_group(&outer, &inner);
        use_both(&outer, &inner);       // ✅ §5.8 无返回值
        let _ = two_ref(&outer, &inner); // ✅ 返回值仅在内层丢弃
        demo(&outer, &inner);           // ✅ 空函数体，内层调用
        // same_group_elision(&outer, &inner);
        {
            let a = 1;
            let b = 2;
            print_both(&a, &b);
        }
    }
    println!("出内层块后 outer 原变量仍可用: {}", outer);
    let r = &outer; // ✅ 新建普通引用，全新生命周期，与旧 'a 无关
    println!("出内层块后新建普通引用: {}", r);

    triple(&v1, &v2, &num);
    println!("pick_one: {}", pick_one(&v1, &v2));
    let (rx, ry) = return_two(&v1, &v2);
    println!("return_two: ({}, {})", rx, ry);
    println!("pick_any max: {}", pick_any(&v1, &v2));

    println!("\n=== 0.9) 'a 是函数局部标签 ===");
    demo1(&v1, &v2);
    demo2(&v1);
    let ret = get_short(&v1, &v2);
    println!("get_short 返回: {}", ret);
    {
        let inner = 99;
        demo2(&inner);
        let r1 = return_ref(&inner);
        println!("return_ref(&inner): {}", r1);
        // demo1(&outer, &inner); // ❌ 同函数内共用 'a
    }
    demo2(&outer);
    let r2 = return_ref(&outer);
    println!("return_ref(&outer): {}", r2);

    println!("\n=== 1) get_first：返回引用与入参同 'a ===");
    let s = String::from("中文abc");
    let first = get_first(&s);
    println!("get_first(\"中文abc\") = {:?}", first);

    println!("\n=== 2) longest：同块调用，'a 动态适配 ===");
    let string1 = String::from("abcd");
    let string2 = "xyz";
    let result = longest(string1.as_str(), string2);
    println!("longest = {}", result);

    println!("\n=== 3) longest 一长一短（§8.4 场景 A：引用在红线内）===");
    let string1 = String::from("long string is long");
    {
        let string2 = String::from("xyz");
        let result = longest(string1.as_str(), string2.as_str());
        println!("场景 A: {}", result);
    }
    // 场景 B ❌ res 在外层（取消注释即编译失败）：
    // let res: &str;
    // { let inner = String::from("short"); res = longest(string1.as_str(), inner.as_str()); }
    // 补充 ❌ 出红线后 println!(res)

    println!("\n=== 3b) two_str：'a/'b 分组，一长一短合法 ===");
    let outer = String::from("outer");
    {
        let inner = String::from("inner");
        two_str(outer.as_str(), inner.as_str());
    }

    println!("\n=== 4) 结构体 ImportantExcerpt<'a> ===");
    let novel = String::from("Call me Ishmael. Some years ago...");
    let first_sentence = novel
        .split('.')
        .next()
        .expect("Could not find a '.'");
    let i = ImportantExcerpt {
        part: first_sentence,
    };
    println!("level = {}", i.level());
    println!(
        "part = {}",
        i.announce_and_return_part("hello lifetimes")
    );

    println!("\n=== 5) 'static ===");
    let s: &'static str = "I have a static lifetime.";
    println!("{}", s);

    println!("\n=== 6) 泛型 + trait bound + 生命周期 ===");
    let x = "short";
    let y = "a bit longer";
    let r = longest_with_an_announcement(x, y, 123);
    println!("result = {}", r);
}
