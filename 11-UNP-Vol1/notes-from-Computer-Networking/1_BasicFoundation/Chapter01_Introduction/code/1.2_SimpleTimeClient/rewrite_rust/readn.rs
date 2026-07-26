//! 对标 UNP Ch 3.9 `readn`：读满 n 字节或遇 EOF/错误返回（处理 EINTR 重试）
//!
//! 在 daytimetcpcli 中可将 `stream.read(&mut buf)?` 换为：
//!   `readn::readn(&mut stream, &mut buf, MAXLINE)?`（语义略不同：readn 尽量读满缓冲区）

use std::io::{self, Read};

/// 从 `r` 读入 `buf` 的前 `n` 个字节，遇 EINTR 自动重试。
/// 返回：已读字节数；对端提前关闭时可能 `< n` 且下次再读得 0。
pub fn readn<R: Read>(r: &mut R, buf: &mut [u8], n: usize) -> io::Result<usize> {
    let mut nread = 0;
    while nread < n {
        match r.read(&mut buf[nread..n]) {
            Ok(0) => return Ok(nread),
            Ok(m) => nread += m,
            Err(ref e) if e.kind() == io::ErrorKind::Interrupted => continue,
            Err(e) => return Err(e),
        }
    }
    Ok(nread)
}
