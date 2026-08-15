//! 18.3 模式语法 demo：字面量、解构、忽略符、守卫、@ 绑定

// ── 类型定义 ────────────────────────────────────────────────────────────────

pub struct Point {
    pub x: i32,
    pub y: i32,
}

pub struct Person {
    pub name: String,
    pub age: u32,
    pub addr: String,
}

pub enum Color {
    Rgb(u8, u8, u8),
    Hsv(u8, u8, u8),
    Black,
}

pub enum Msg {
    ChangeColor(Color),
}

pub enum Status {
    Code(i32),
}

pub enum HelloMsg {
    Hello { id: i32 },
}

// ── §二 字面量 + | + ..= ────────────────────────────────────────────────────

pub fn demo_literals_or_range() {
    let num = 2;
    match num {
        1 => println!("  字面量：1"),
        2 => println!("  字面量：2"),
        _ => println!("  字面量：其他"),
    }

    let n = 3;
    match n {
        1 | 2 | 3 => println!("  多模式 |：匹配 1/2/3"),
        _ => (),
    }

    let age = 22;
    match age {
        0..=18 => println!("  范围：未成年"),
        19..=60 => println!("  范围：成年"),
        _ => println!("  范围：长者"),
    }

    let ch = 'd';
    match ch {
        'a'..='z' => println!("  范围：小写字母"),
        'A'..='Z' => println!("  范围：大写字母"),
        _ => (),
    }
}

// ── §三 match 变量遮蔽 ───────────────────────────────────────────────────────

pub fn demo_match_shadowing() {
    let y = 10;
    let opt = Some(10);

    match opt {
        Some(y) => println!("  遮蔽：分支内 y = {y}（新绑定）"),
        None => (),
    }
    println!("  遮蔽：外层 y = {y}（不受影响）");
}

// ── §五 解构 ─────────────────────────────────────────────────────────────────

pub fn demo_destructure() {
    let t = (10, 20);
    let (a, b) = t;
    println!("  元组解构：a={a}, b={b}");

    match t {
        (0, y) => println!("  元组 match：(0, {y})"),
        (x, 20) => println!("  元组 match：({x}, 20)"),
        _ => (),
    }

    let p = Point { x: 0, y: 5 };
    let Point { x, y } = p;
    println!("  结构体简写：x={x}, y={y}");

    match p {
        Point { x: 0, y } => println!("  结构体字面量：x=0, y={y}"),
        Point { x, y: 0 } => println!("  结构体字面量：y=0, x={x}"),
        _ => (),
    }

    let c = Color::Rgb(255, 0, 0);
    match c {
        Color::Rgb(r, g, b) => println!("  枚举 RGB：{r} {g} {b}"),
        Color::Hsv(h, s, v) => println!("  枚举 HSV：{h} {s} {v}"),
        Color::Black => println!("  枚举：Black"),
    }

    let msg = Msg::ChangeColor(Color::Hsv(10, 20, 30));
    match msg {
        Msg::ChangeColor(Color::Hsv(h, s, v)) => {
            println!("  嵌套解构 HSV：{h} {s} {v}");
        }
        _ => (),
    }

    let ((feet, inches), Point { x, y }) = ((3, 10), Point { x: 3, y: -10 });
    println!("  混合解构：feet={feet}, inches={inches}, x={x}, y={y}");
}

// ── §六 忽略符 _ / .. ───────────────────────────────────────────────────────

pub fn demo_ignore_underscore() {
    let s = Some(String::from("hello"));
    if let Some(_) = s {
        println!("  _ ：匹配成功，s 仍可用");
    }
    println!("  _ ：s = {s:?}");

    fn calc(_: i32, b: i32) -> i32 {
        b * 2
    }
    println!("  _ 参数：calc(99, 4) = {}", calc(99, 4));

    let p = Person {
        name: "Tom".into(),
        age: 20,
        addr: "Beijing".into(),
    };
    let Person { name, .. } = p;
    println!("  .. 结构体：name = {name}");

    let t = (1, 2, 3, 4, 5);
    let (first, .., last) = t;
    println!("  .. 元组：first={first}, last={last}");

    let numbers = (2, 4, 8, 16, 32);
    match numbers {
        (first, _, third, _, fifth) => {
            println!("  _ 元组：{first}, {third}, {fifth}");
        }
    }
}

/// `_s` 会 move — 此处用注释说明，避免 demo 自身编译失败
pub fn demo_underscore_prefix_moves() {
    println!("  _s：if let Some(_s) = s 会绑定并 move String");
    println!("  → 之后 println!(\"{{:?}}\", s) 报 E0382: borrow of moved value");
    println!("  对比：if let Some(_) = s 不绑定，s 仍可用（见 demo_ignore_underscore）");
}

// ── §七 匹配守卫 ─────────────────────────────────────────────────────────────

pub fn demo_match_guards() {
    let y = 5;
    let opt = Some(5);
    match opt {
        Some(n) if n == y => println!("  守卫：n 等于外层 y"),
        Some(n) => println!("  守卫：n = {n}"),
        None => (),
    }

    let num = 2;
    let flag = true;
    match num {
        1 | 2 | 3 if flag => println!("  守卫+|：1/2/3 且 flag=true"),
        _ => println!("  守卫+|：不匹配"),
    }

    let num = Some(4);
    match num {
        Some(x) if x < 5 => println!("  守卫：小于 5 → {x}"),
        Some(x) => println!("  守卫：{x}"),
        None => (),
    }
}

// ── §八 @ 绑定 ───────────────────────────────────────────────────────────────

pub fn demo_at_binding() {
    let n = 5;
    match n {
        id @ 3..=7 => println!("  @ 范围：{id} 在 3~7"),
        _ => println!("  @ 范围：超出"),
    }

    let s = Status::Code(200);
    match s {
        Status::Code(val @ 200..=299) => println!("  @ 枚举：正常状态码 {val}"),
        _ => println!("  @ 枚举：异常"),
    }

    let msg = HelloMsg::Hello { id: 5 };
    match msg {
        HelloMsg::Hello {
            id: id @ 3..=7,
        } => println!("  @ 结构体字段：id = {id}"),
        HelloMsg::Hello { id } => println!("  @ 结构体字段：其他 id = {id}"),
    }
}

// ── 整合 demo ───────────────────────────────────────────────────────────────

pub fn demo_all_pattern_syntax() {
    println!("--- 字面量 | 范围 ---");
    demo_literals_or_range();

    println!("\n--- 变量遮蔽 ---");
    demo_match_shadowing();

    println!("\n--- 解构 ---");
    demo_destructure();

    println!("\n--- 忽略 _ / .. ---");
    demo_ignore_underscore();

    println!("\n--- 匹配守卫 ---");
    demo_match_guards();

    println!("\n--- @ 绑定 ---");
    demo_at_binding();
}

pub fn demo_compile_error_notes() {
    println!("  【易错 1】match 分支变量遮蔽外层同名变量");
    println!("    Some(y) 内 y 是新绑定；引用外层 y → Some(n) if n == y");
    println!();
    println!("  【易错 2】_ vs _x 所有权");
    println!("    if let Some(_) = s   → s 仍可用");
    println!("    if let Some(_s) = s  → E0382: s 被 move");
    println!();
    println!("  【易错 3】元组两个 ..");
    println!("    let (.., mid, ..) = t  → E0425: only one .. allowed");
    println!();
    println!("  【易错 4】浮点范围模式");
    println!("    match x {{ 1.0..=5.0 => ... }}  → 不支持，仅整数/char");
    println!();
    println!("  【易错 5】守卫 + | 作用于整组");
    println!("    1 | 2 | 3 if flag  → flag 对 1、2、3 全部生效");
    println!();
    println!("  【易错 6】match 未穷尽 → E0004，用 _ 兜底");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn tuple_destructure() {
        let (a, b) = (1, 2);
        assert_eq!(a + b, 3);
    }

    #[test]
    fn underscore_no_move() {
        let s = Some(String::from("x"));
        if let Some(_) = s {
            // ok
        }
        assert!(s.is_some());
    }

    #[test]
    fn at_binding_range() {
        let n = 5;
        let matched = match n {
            id @ 3..=7 => Some(id),
            _ => None,
        };
        assert_eq!(matched, Some(5));
    }

    #[test]
    fn guard_with_outer_var() {
        let y = 10;
        let opt = Some(10);
        let ok = match opt {
            Some(n) if n == y => true,
            _ => false,
        };
        assert!(ok);
    }

    #[test]
    fn or_pattern() {
        let n = 2;
        let hit = matches!(n, 1 | 2 | 3);
        assert!(hit);
    }
}
