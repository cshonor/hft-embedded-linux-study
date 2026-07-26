//! UNP 1.5 · Daytime TCP 迭代服务器（对齐 C daytimesrv.c）
//!
//!   cargo run
//! 客户端：1.2 daytimetcpcli / cargo run -- 127.0.0.1

use std::io::Write;
use std::net::TcpListener;

fn main() -> std::io::Result<()> {
    /* bind + listen 合并：0.0.0.0:13 等价 INADDR_ANY + htons(13) */
    let listener = TcpListener::bind("0.0.0.0:13")?;
    eprintln!("daytime server listening on 0.0.0.0:13");

    /* 迭代：loop + accept 等价 C for(;;) Accept */
    loop {
        let (mut conn, peer) = listener.accept()?;
        eprintln!("accepted from {peer}");

        /* {:?} 需要 Debug trait；SystemTime 已实现 Debug（非 Display，见 1.2_Appendix_Rust_Debug_Display与Trait.md） */
        let time_str = format!("{:?}\r\n", std::time::SystemTime::now());
        conn.write_all(time_str.as_bytes())?;
        /* conn Drop → close(connfd)；listener 仍存活 ≡ listenfd */
    }
}
