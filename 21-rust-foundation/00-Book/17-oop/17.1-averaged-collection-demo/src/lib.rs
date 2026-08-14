//! 17.1 面向对象三大特征 demo：对象、封装、继承替代、多态

// ── 1. 封装：AveragedCollection ─────────────────────────────────────────────

/// 带平均值缓存的数字集合（封装示例）
#[derive(Debug, Default)]
pub struct AveragedCollection {
    list: Vec<i32>,
    average: f64,
}

impl AveragedCollection {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn add(&mut self, value: i32) {
        self.list.push(value);
        self.update_average();
    }

    pub fn remove(&mut self) -> Option<i32> {
        let val = self.list.pop();
        if val.is_some() {
            self.update_average();
        }
        val
    }

    pub fn average(&self) -> f64 {
        self.average
    }

    pub fn len(&self) -> usize {
        self.list.len()
    }

    fn update_average(&mut self) {
        let total: i32 = self.list.iter().sum();
        self.average = if self.list.is_empty() {
            0.0
        } else {
            total as f64 / self.list.len() as f64
        };
    }
}

// ── 2. 对象：数据 + 行为 ───────────────────────────────────────────────────

pub struct Book {
    title: String,
    pages: u32,
}

impl Book {
    pub fn new(title: &str, pages: u32) -> Self {
        Self {
            title: title.to_string(),
            pages,
        }
    }

    pub fn info(&self) -> String {
        format!("《{}》共{}页", self.title, self.pages)
    }
}

// ── 3. 继承替代 + 多态：Summary trait ──────────────────────────────────────

pub trait Summary {
    fn summarize(&self) -> String {
        String::from("默认摘要")
    }
}

pub struct News {
    pub headline: String,
}

impl Summary for News {
    fn summarize(&self) -> String {
        format!("新闻：{}", self.headline)
    }
}

pub struct Tweet {
    pub username: String,
    pub content: String,
}

impl Summary for Tweet {
    fn summarize(&self) -> String {
        format!("@{}: {}", self.username, self.content)
    }
}

/// 静态分发（编译期单态化）
pub fn print_summary<T: Summary>(item: &T) {
    println!("  [静态] {}", item.summarize());
}

/// 动态分发（trait 对象）
pub fn print_all(items: &[&dyn Summary]) {
    for item in items {
        println!("  [动态] {}", item.summarize());
    }
}

// ── 整合 demo ───────────────────────────────────────────────────────────────

pub fn demo_all_oop_features() {
    // 封装
    let mut coll = AveragedCollection::new();
    coll.add(10);
    coll.add(20);
    coll.add(30);
    println!("  AveragedCollection：len={} avg={}", coll.len(), coll.average());
    coll.remove();
    println!("  remove 后：len={} avg={}", coll.len(), coll.average());

    // 对象
    let book = Book::new("Rust 程序设计", 560);
    println!("  Book：{}", book.info());

    // 多态
    let news = News {
        headline: "Rust 1.87 发布".into(),
    };
    let tweet = Tweet {
        username: "rustlang".into(),
        content: "Hello!".into(),
    };
    print_summary(&news);
    print_summary(&tweet);
    print_all(&[&news, &tweet]);
}

pub fn demo_traditional_vs_rust_notes() {
    println!("  传统 OOP          →  Rust");
    println!("  ─────────────────────────────────────");
    println!("  对象=数据+方法    →  struct/enum + impl");
    println!("  封装=private      →  默认私有 + pub API");
    println!("  继承=extends      →  无语法；Trait 默认方法 + 组合");
    println!("  多态=虚函数       →  T: Trait（静态）/ dyn Trait（动态）");
    println!();
    println!("  易错：");
    println!("  · coll.list 编译失败 — 私有字段不可外部访问");
    println!("  · struct B : A 不存在 — 用嵌套 struct 组合字段");
    println!("  · 默认方法只复用行为，不复用字段");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn average_tracks_list() {
        let mut c = AveragedCollection::new();
        c.add(10);
        c.add(20);
        c.add(30);
        assert!((c.average() - 20.0).abs() < f64::EPSILON);
        c.remove();
        assert!((c.average() - 15.0).abs() < f64::EPSILON);
    }

    #[test]
    fn book_info() {
        let b = Book::new("Demo", 100);
        assert!(b.info().contains("Demo"));
    }

    #[test]
    fn summary_polymorphism() {
        let news = News {
            headline: "x".into(),
        };
        assert!(news.summarize().starts_with("新闻"));
    }

    // 编译报错：list/average 是私有字段
    // fn access_private() {
    //     let coll = AveragedCollection::new();
    //     let _ = coll.list;
    // }
}
