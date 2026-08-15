//! 3.6 Creating Join Macro：书中风格 `join!`（顺序 `block_on`）+ `flume` 收发。

macro_rules! join {
    ($($future:expr),*) => {{
        let mut results = Vec::new();
        $(
            results.push(futures_lite::future::block_on($future));
        )*
        results
    }};
}

async fn task_one() -> u32 {
    10
}

async fn task_two() -> u32 {
    20
}

fn main() {
    let outcome: Vec<u32> = join!(task_one(), task_two());
    println!("join! (sequential block_on): {:?}", outcome);

    let (tx, rx) = flume::bounded::<u32>(4);
    tx.send(1).expect("send");
    tx.send(2).expect("send");
    println!("flume recv: {:?}", rx.recv());
    println!("flume recv: {:?}", rx.recv());
}
