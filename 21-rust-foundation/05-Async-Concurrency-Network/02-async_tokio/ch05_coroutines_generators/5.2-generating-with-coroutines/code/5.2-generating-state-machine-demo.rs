//! 5.2 Generating：stable 状态机模拟 Yield/Resume（惰性逐行，对照 Generator）。

#[derive(Debug)]
enum Step {
    Yielded(i32),
    Complete,
}

struct LineParser {
    lines: &'static [&'static str],
    i: usize,
}

impl LineParser {
    fn resume(&mut self) -> Step {
        if self.i >= self.lines.len() {
            return Step::Complete;
        }
        let v: i32 = self.lines[self.i].parse().unwrap_or(0);
        self.i += 1;
        Step::Yielded(v)
    }
}

fn main() {
    let mut gen = LineParser {
        lines: &["1", "2", "3"],
        i: 0,
    };
    while let Step::Yielded(n) = gen.resume() {
        println!("parsed: {n}");
    }
    println!("=== 5.2 generator complete ===");
}
