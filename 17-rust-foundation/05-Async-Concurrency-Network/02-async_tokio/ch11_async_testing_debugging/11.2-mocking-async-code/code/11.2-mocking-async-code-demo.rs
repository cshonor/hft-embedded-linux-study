//! §11.2：每测独立 `current_thread` + 手写 async trait 替身（理解 mock 异步依赖）。

trait Greeter {
    fn greet(&self, name: &str) -> impl std::future::Future<Output = String> + Send;
}

struct FakeGreeter;
impl Greeter for FakeGreeter {
    fn greet(&self, name: &str) -> impl std::future::Future<Output = String> + Send {
        let name = name.to_string();
        async move { format!("hello, {name}") }
    }
}

async fn use_greeter(g: &impl Greeter) -> String {
    g.greet("world").await
}

fn run_test() {
    let rt = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .expect("runtime");
    let out = rt.block_on(use_greeter(&FakeGreeter));
    assert_eq!(out, "hello, world");
}

fn main() {
    run_test();
    println!("§11.2 ok — 生产中用 mockall 替换 FakeGreeter");
}
