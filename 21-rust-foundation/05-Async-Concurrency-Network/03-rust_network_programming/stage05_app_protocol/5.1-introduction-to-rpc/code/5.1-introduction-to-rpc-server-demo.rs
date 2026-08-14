//! §5.1：最小 JSON-RPC 形态（Serde + TCP），理解 stub 调用链。
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;

use serde::{Deserialize, Serialize};

const ADDR: &str = "127.0.0.1:48081";

#[derive(Debug, Deserialize)]
struct RpcRequest {
    method: String,
    a: i32,
    b: i32,
}

#[derive(Debug, Serialize)]
struct RpcResponse {
    result: i32,
}

fn handle(mut stream: TcpStream) -> std::io::Result<()> {
    let mut buf = [0u8; 1024];
    let n = stream.read(&mut buf)?;
    let req: RpcRequest = serde_json::from_slice(&buf[..n])?;
    let resp = if req.method == "add" {
        RpcResponse {
            result: req.a + req.b,
        }
    } else {
        RpcResponse { result: -1 }
    };
    stream.write_all(&serde_json::to_vec(&resp)?)?;
    Ok(())
}

fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind(ADDR)?;
    println!("5.1 RPC server on {ADDR}");
    for stream in listener.incoming() {
        let stream = stream?;
        thread::spawn(move || {
            let _ = handle(stream);
        });
    }
    Ok(())
}
