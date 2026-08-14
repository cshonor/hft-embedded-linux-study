// 10.2.4 type 关键字 — 类型别名 · 关联类型 · 易混区分

use std::fs::File;
use std::io::Error;

// ── 场景 1：普通类型别名 ─────────────────────────────
type Num = i32;
type FileResult = Result<File, Error>;
type Pair<T> = (T, T);

fn open_file(path: &str) -> FileResult {
    File::open(path)
}

// ── 场景 2：Trait 内关联类型 ─────────────────────────
trait MyIterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>;
}

struct Counter {
    idx: u32,
}

impl MyIterator for Counter {
    type Item = u32;

    fn next(&mut self) -> Option<Self::Item> {
        if self.idx < 3 {
            let val = self.idx;
            self.idx += 1;
            Some(val)
        } else {
            None
        }
    }
}

// ── 场景 4：type vs struct（新类型）──────────────────
type Age = i32;

struct NewAge(i32);

fn main() {
    println!("=== §1 普通 type 别名 ===");
    let a: Num = 100;
    let b: i32 = a;
    println!("  Num={a}, i32={b}");

    let p1: Pair<i32> = (1, 2);
    let p2: Pair<String> = ("a".into(), "b".into());
    println!("  Pair<i32>={p1:?}, Pair<String>={p2:?}");

    match open_file("/nonexistent/path/for/demo") {
        Ok(_) => println!("  FileResult: opened"),
        Err(e) => println!("  FileResult: Err({e})"),
    }

    println!("\n=== §2 Trait 内 type Item（关联类型）===");
    let mut c = Counter { idx: 0 };
    print!("  Counter: ");
    while let Some(n) = c.next() {
        print!("{n} ");
    }
    println!();

    println!("\n=== §3 type Age vs struct NewAge ===");
    let age: Age = 18;
    let as_i32: i32 = age;
    println!("  type Age={age} → i32={as_i32} ✅");

    let wrapped = NewAge(18);
    println!("  struct NewAge(18) = NewAge({})", wrapped.0);
    // let _bad: i32 = wrapped; // ❌ 不同类型

    println!("\nok: type 关键字 demo 完成");
}
