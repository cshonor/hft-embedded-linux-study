//! §6.4：同步回调最小例（完整键盘输入见书中 `device_query` + 事件循环）。

fn perform_operation_with_callback<F>(callback: F)
where
    F: Fn(i32),
{
    let result = 42;
    callback(result);
}

fn main() {
    let my_callback = |result: i32| {
        println!("回调收到: {result}");
    };
    perform_operation_with_callback(my_callback);
}
