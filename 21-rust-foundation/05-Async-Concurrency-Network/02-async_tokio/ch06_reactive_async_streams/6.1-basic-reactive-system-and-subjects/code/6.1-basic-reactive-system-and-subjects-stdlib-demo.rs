//! §6.1：定义加热器「主题」——`AtomicI16` / `AtomicBool` + `LazyLock`（无 Tokio）。
#![crate_name = "demo_6_1_subjects"]
use std::sync::atomic::{AtomicBool, AtomicI16, Ordering};
use std::sync::LazyLock;

/// 当前温度（主题）
static TEMP: AtomicI16 = AtomicI16::new(18);
/// 目标温度（主题，Lazy 初始化示意书中全局配置）
static DESIRED_TEMP: LazyLock<AtomicI16> = LazyLock::new(|| AtomicI16::new(22));
/// 加热器开关（主题）
static HEATER_ON: AtomicBool = AtomicBool::new(false);

fn main() {
    println!(
        "Subjects snapshot: temp={}°C desired={}°C heater_on={}",
        TEMP.load(Ordering::Relaxed),
        DESIRED_TEMP.load(Ordering::Relaxed),
        HEATER_ON.load(Ordering::Relaxed)
    );
    DESIRED_TEMP.store(24, Ordering::Relaxed);
    println!(
        "After desired=24: temp={} desired={}",
        TEMP.load(Ordering::Relaxed),
        DESIRED_TEMP.load(Ordering::Relaxed)
    );
}
