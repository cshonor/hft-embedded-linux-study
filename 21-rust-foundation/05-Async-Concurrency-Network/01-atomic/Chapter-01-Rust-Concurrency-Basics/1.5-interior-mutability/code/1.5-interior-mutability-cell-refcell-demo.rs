use std::cell::{Cell, RefCell};

/// 演示 Cell 的基本使用：适用于 Copy 类型
fn demo_cell_basic() {
    println!("=== Cell 基本使用示例 ===");
    
    // Cell 适用于 Copy 类型（如 i32）
    let cell = Cell::new(5);
    let ref_cell: &Cell<i32> = &cell; // 拿到 Cell 的不可变引用
    
    // 通过不可变引用修改内部值
    ref_cell.set(10);
    println!("通过不可变引用 set(10) 后，值: {}", ref_cell.get());
    
    // get 返回的是复制值，不会影响 Cell 内部
    let copy1 = cell.get(); // 复制出 10
    cell.set(20); // 修改内部值为 20
    println!("copy1 的值: {} (不受 set 影响)", copy1);
    println!("cell 内部的值: {}", cell.get());
    
    println!();
}

/// 演示 Cell 的 take 方法
fn demo_cell_take() {
    println!("=== Cell::take() 方法示例 ===");
    
    let cell = Cell::new(42);
    
    // take 拿走内部值，Cell 变成默认值（i32 默认是 0）
    let mut v2 = cell.take(); // v2 的类型是 i32，值是 42
    println!("take 拿到的值: {}", v2);
    println!("take 后 cell 的值（默认值）: {}", cell.get());
    
    // 修改拿到的值
    v2 += 8; // 现在 v2 是 50
    
    // 可以再把修改后的值 set 回去
    cell.set(v2);
    println!("修改后 set 回去，cell 的值: {}", cell.get());
    
    println!();
}

/// 演示 Cell 解决"可变引用和不可变引用不能共存"的问题
fn demo_cell_solve_borrow_conflict() {
    println!("=== Cell 解决借用冲突示例 ===");
    
    struct Counter {
        count: Cell<u32>, // 用 Cell 包裹，允许通过不可变引用修改
    }
    
    impl Counter {
        fn new() -> Self {
            Counter { count: Cell::new(0) }
        }
        
        // 注意：这里用的是 &self（不可变引用），不是 &mut self
        fn increment(&self) {
            let current = self.count.get();
            self.count.set(current + 1);
        }
        
        fn get_count(&self) -> u32 {
            self.count.get()
        }
    }
    
    let counter = Counter::new();
    let ref1 = &counter; // 不可变引用
    let ref2 = &counter; // 另一个不可变引用，可以共存！
    
    // 通过不可变引用修改内部状态
    ref1.increment();
    ref2.increment();
    
    println!("计数器的值: {}", counter.get_count()); // 输出 2
    
    println!();
}

/// 演示 RefCell 的基本使用：适用于非 Copy 类型
fn demo_refcell_basic() {
    println!("=== RefCell 基本使用示例 ===");
    
    // RefCell 可以包裹非 Copy 类型（如 String、Vec）
    let ref_cell = RefCell::new(String::from("hello"));
    
    // borrow() 拿到不可变引用（Ref<String>）
    let borrow1 = ref_cell.borrow();
    println!("不可变借用1: {}", *borrow1);
    
    // 可以同时有多个不可变借用
    let borrow2 = ref_cell.borrow();
    println!("不可变借用2: {}", *borrow2);
    
    // 释放借用（离开作用域）
    drop(borrow1);
    drop(borrow2);
    
    // 现在可以拿到可变借用
    {
        let mut borrow_mut = ref_cell.borrow_mut();
        borrow_mut.push_str(" world");
        println!("修改后的值: {}", *borrow_mut);
    } // borrow_mut 离开作用域，自动释放
    
    println!("最终值: {}", ref_cell.borrow());
    
    println!();
}

/// 演示 RefCell 对基本类型和数组的操作
fn demo_refcell_operations() {
    println!("=== RefCell 操作示例 ===");
    
    // 1. 基本类型（i32）的操作
    let ref_cell_i32 = RefCell::new(5);
    {
        let mut num = ref_cell_i32.borrow_mut();
        *num += 3; // 加等于
        *num -= 1; // 减等于
    }
    println!("i32 操作后: {}", ref_cell_i32.borrow());
    
    // 2. 数组（Vec）的操作
    let ref_cell_vec = RefCell::new(vec![1, 2, 3]);
    {
        let mut vec = ref_cell_vec.borrow_mut();
        vec.push(4); // push 操作
        vec.push(5);
    }
    println!("Vec 操作后: {:?}", ref_cell_vec.borrow());
    
    println!();
}

/// 演示 RefCell 的借用规则（运行时检查）
fn demo_refcell_borrow_rules() {
    println!("=== RefCell 借用规则示例 ===");
    
    let ref_cell = RefCell::new(42);
    
    // 正确用法：用大括号控制作用域
    {
        let mut borrow1 = ref_cell.borrow_mut();
        *borrow1 = 100;
    } // borrow1 离开作用域，释放借用
    
    // 现在可以再次借用
    {
        let mut borrow2 = ref_cell.borrow_mut();
        *borrow2 = 200;
    }
    
    println!("最终值: {}", ref_cell.borrow());
    
    // 错误用法（会 panic，这里注释掉）：
    // let mut borrow1 = ref_cell.borrow_mut();
    // let mut borrow2 = ref_cell.borrow_mut(); // panic! 同一时间不能有两个可变借用
    
    println!();
}

/// 演示 Cell 和 RefCell 的区别总结
fn demo_comparison() {
    println!("=== Cell vs RefCell 总结 ===");
    println!("Cell:");
    println!("  - 只适用于 Copy 类型（i32, bool, f64 等）");
    println!("  - get() 返回复制值，set() 直接覆盖");
    println!("  - 没有运行时开销（直接内存操作）");
    println!("  - 单线程使用");
    println!();
    println!("RefCell:");
    println!("  - 适用于所有类型（包括 String, Vec 等非 Copy 类型）");
    println!("  - borrow() 返回不可变引用，borrow_mut() 返回可变引用");
    println!("  - 运行时借用检查（违反规则会 panic）");
    println!("  - 单线程使用");
    println!();
    println!("共同点:");
    println!("  - 都实现内部可变性（通过不可变引用修改内部数据）");
    println!("  - 都只能在单线程使用（不实现 Sync trait）");
    println!("  - 都解决了\"可变引用和不可变引用不能共存\"的问题");
}

pub fn main() {
    demo_cell_basic();
    demo_cell_take();
    demo_cell_solve_borrow_conflict();
    demo_refcell_basic();
    demo_refcell_operations();
    demo_refcell_borrow_rules();
    demo_comparison();
}

