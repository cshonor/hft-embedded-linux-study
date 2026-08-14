//! 17.2 Trait 对象 demo：`Draw` + `Screen` + 枚举/泛型对照 + 对象安全 + 易错点

// ── 枚举方案（固定类型，不可扩展）────────────────────────────────────────

pub enum UiComponent {
    Button(Button),
    TextField(TextField),
}

impl UiComponent {
    pub fn draw(&self) {
        match self {
            UiComponent::Button(b) => b.draw(),
            UiComponent::TextField(t) => t.draw(),
        }
    }
}

// ── Draw trait + 组件 ─────────────────────────────────────────────────────

pub trait Draw {
    fn draw(&self);
}

pub struct Button {
    pub label: String,
}

impl Draw for Button {
    fn draw(&self) {
        println!("  绘制按钮：{}", self.label);
    }
}

pub struct TextField {
    pub content: String,
}

impl Draw for TextField {
    fn draw(&self) {
        println!("  绘制文本框：{}", self.content);
    }
}

/// 未实现 Draw — 演示「未 impl trait 不能装箱」
pub struct Image {
    pub path: String,
}

// ── Trait 对象：异质 Screen ─────────────────────────────────────────────────

pub struct Screen {
    pub components: Vec<Box<dyn Draw>>,
}

impl Screen {
    pub fn new() -> Self {
        Self {
            components: Vec::new(),
        }
    }

    pub fn add_component(&mut self, comp: Box<dyn Draw>) {
        self.components.push(comp);
    }

    pub fn run(&self) {
        for comp in &self.components {
            comp.draw();
        }
    }
}

// ── 泛型：同质 Screen（对比用）──────────────────────────────────────────────

pub struct HomogeneousScreen<T: Draw> {
    pub components: Vec<T>,
}

impl<T: Draw> HomogeneousScreen<T> {
    pub fn run(&self) {
        for comp in &self.components {
            comp.draw();
        }
    }
}

// ── 可变 trait 对象：Count ───────────────────────────────────────────────────

pub trait Count {
    fn inc(&mut self);
    fn get(&self) -> u32;
}

pub struct Counter {
    pub num: u32,
}

impl Count for Counter {
    fn inc(&mut self) {
        self.num += 1;
    }
    fn get(&self) -> u32 {
        self.num
    }
}

pub fn demo_mutable_trait_object() {
    let mut c = Counter { num: 0 };
    let obj: &mut dyn Count = &mut c;
    obj.inc();
    obj.inc();
    println!("  Counter via &mut dyn Count: {}", obj.get());
}

// ── 整合 demo ───────────────────────────────────────────────────────────────

pub fn demo_gui_trait_objects() {
    let mut screen = Screen::new();

    screen.add_component(Box::new(Button {
        label: "确定".into(),
    }));
    screen.add_component(Box::new(TextField {
        content: "请输入内容".into(),
    }));

    println!("=== 初始组件渲染 ===");
    screen.run();
}

pub fn demo_gui_with_extension<F>(add_select: F)
where
    F: FnOnce(&mut Screen),
{
    let mut screen = Screen::new();
    screen.add_component(Box::new(Button {
        label: "确定".into(),
    }));
    screen.add_component(Box::new(TextField {
        content: "请输入内容".into(),
    }));
    add_select(&mut screen);
    println!("\n=== 新增组件后渲染 ===");
    screen.run();
}

pub fn demo_enum_vs_dyn_notes() {
    println!("  【枚举】UiComponent — 变体固定，新增 SelectBox 须改枚举 + 重编译");
    let items = vec![
        UiComponent::Button(Button {
            label: "OK".into(),
        }),
        UiComponent::TextField(TextField {
            content: "hi".into(),
        }),
    ];
    for item in &items {
        item.draw();
    }
    println!();
    println!("  【Trait 对象】Screen — Vec<Box<dyn Draw>> 异质混存，使用者 crate 扩展类型");
}

pub fn demo_generic_vs_dyn_notes() {
    println!("  泛型 HomogeneousScreen<T>     Trait 对象 Screen");
    println!("  ─────────────────────────────────────────────────");
    println!("  Vec<T> 同质（全 Button 或全 TextField）");
    println!("  静态分发 · 零运行时开销 · 可内联");
    println!("  不能混放多种类型");
    println!();
    println!("                              Vec<Box<dyn Draw>> 异质");
    println!("                              动态分发 · vtable 间接调用");
    println!("                              可混放 + 外部扩展（SelectBox）");
    println!();
    println!("  口诀：固定已知求性能 → 泛型；需扩展混存 → dyn Trait + Box");
}

pub fn demo_object_safety_notes() {
    println!("  对象安全（Object Safe）— 才能 dyn Trait");
    println!("  ─────────────────────────────────────────");
    println!("  1. 方法不能返回 Self     → Clone::clone() -> Self  → E0038 dyn Clone 非法");
    println!("  2. 方法不能有独立泛型     → fn foo<T>(&self, x: T)  → E0038 非对象安全");
    println!("  3. 补充：async fn / 无 &self 关联函数 一般也不能通过 trait 对象调用");
    println!();
    println!("  口诀：有返回 Self、有独立泛型、有 async → 不能 dyn Trait");
}

pub fn demo_compile_error_notes() {
    println!("  【易错 1】dyn Trait 是 DST，不能单独使用");
    println!("    ❌ let v: Vec<dyn Draw> = Vec::new();");
    println!("    → E0277: size cannot be known at compilation time");
    println!("    ✅ let v: Vec<Box<dyn Draw>> = Vec::new();");
    println!();
    println!("  【易错 2】泛型 Vec<T> 不能混存多种具体类型");
    println!("    ❌ HomogeneousScreen {{ components: vec![Button{{..}}, TextField{{..}}] }}");
    println!("    → E0308: mismatched types");
    println!("    ✅ Screen {{ components: vec![Box::new(Button{{..}}), Box::new(TextField{{..}})] }}");
    println!();
    println!("  【易错 3】未 impl Draw 的类型不能装箱");
    println!("    ❌ screen.add_component(Box::new(Image {{ path: ... }}));");
    println!("    → E0277: trait `Draw` is not implemented for `Image`");
    println!();
    println!("  【易错 4】非对象安全 trait");
    println!("    ❌ let x: Box<dyn Clone> = ...;");
    println!("    → E0038: trait `Clone` cannot be made into an object");
    println!();
    println!("  【易错 5】trait 对象 ≠ 继承 — Rust 无 struct 继承，是多态接口 + vtable");
}

// 非对象安全 trait（文档对照）
#[allow(dead_code)]
trait NotObjectSafe {
    fn clone_self(&self) -> Self;
    fn generic_method<T>(&self, _x: T);
}

// let _x: Box<dyn NotObjectSafe>;  // E0038

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn screen_runs_mixed_components() {
        let mut screen = Screen::new();
        screen.add_component(Box::new(Button {
            label: "x".into(),
        }));
        screen.add_component(Box::new(TextField {
            content: "y".into(),
        }));
        assert_eq!(screen.components.len(), 2);
    }

    #[test]
    fn homogeneous_screen_same_type() {
        let screen = HomogeneousScreen {
            components: vec![
                Button {
                    label: "a".into(),
                },
                Button {
                    label: "b".into(),
                },
            ],
        };
        assert_eq!(screen.components.len(), 2);
        screen.run();
    }

    #[test]
    fn enum_draw_delegates() {
        let c = UiComponent::Button(Button {
            label: "test".into(),
        });
        c.draw();
    }

    #[test]
    fn mutable_dyn_count() {
        let mut c = Counter { num: 0 };
        let obj: &mut dyn Count = &mut c;
        obj.inc();
        assert_eq!(obj.get(), 1);
    }

    #[test]
    fn box_dyn_draw_is_valid() {
        let b: Box<dyn Draw> = Box::new(Button {
            label: "boxed".into(),
        });
        b.draw();
    }
}
