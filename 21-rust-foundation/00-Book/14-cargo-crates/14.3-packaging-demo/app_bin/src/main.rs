use common::format_tick;

fn main() {
    println!("{}", format_tick("APP", 1.0));
    println!("[app_bin] 形态1：仅本 Package 出 exe；common 被链接进本进程，函数直调");
}
