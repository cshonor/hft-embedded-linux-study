//! 原子与并发基础：`chapter_01` 线程与同步原语，`chapter_02` 原子与内存序。
//!
//! 源码与《Rust Atomics and Locks》学习文件夹同层。

#[path = "../Chapter-01-Rust-Concurrency-Basics/mod.rs"]
pub mod chapter_01;

#[path = "../Chapter-02-Atomics/mod.rs"]
pub mod chapter_02;

#[path = "../Chapter-03-Memory-Ordering/mod.rs"]
pub mod chapter_03;

#[path = "../Chapter-04-Spin-Locks/mod.rs"]
pub mod chapter_04;

#[path = "../Chapter-05-Channels/mod.rs"]
pub mod chapter_05;

#[path = "../Chapter-06-Custom-Arc/mod.rs"]
pub mod chapter_06;

#[path = "../Chapter-07-Processors/mod.rs"]
pub mod chapter_07;

#[path = "../Chapter-10-Advanced-Concurrent-Data-Structures/mod.rs"]
pub mod chapter_10;

/// 简短概览：与原先 `atomic::demo` 行为一致。
pub fn demo() {
    chapter_02::quick_demo::demo();
}

/// 原 ex1 默认入口：条件变量等示例。
pub fn run_ex1_default() {
    chapter_01::use_condvar::main();
}

/// 原 ex2 中的完整 Atomic 相关演示顺序。
pub fn run_extended() {
    println!("========== 测试 Atomic 类型示例 ==========\n");

    chapter_02::use_atomic::demo_atomic_bool_stop_flag();
    chapter_02::use_atomic::demo_ordering_relaxed();
    chapter_02::use_atomic::demo_release_acquire();
    chapter_02::use_atomic::demo_why_need_release_acquire();
    chapter_02::use_atomic::demo_stdin_control();
    chapter_02::use_atomic::demo_ordering_summary();

    println!("\n========== 测试延迟初始化示例 ==========\n");
    chapter_02::lazy_init::main();

    println!("\n========== 测试原子操作方法 ==========\n");
    chapter_02::use_atomic_operations::main();

    println!("\n========== 测试 ID 分配器 ==========\n");
    chapter_02::id_allocator::main();

    println!("\n========== 测试 SeqCst（顺序一致性）==========\n");
    chapter_03::use_seqcst::main();

    println!("\n========== 测试栅栏（Fence）==========\n");
    chapter_03::use_fence::main();
}
