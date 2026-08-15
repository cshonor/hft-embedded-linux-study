//! §5.1：JSON-RPC 客户端（需 `demo_5_1_rpc_server`）。
use std::io::{Read, Write};
use std::net::TcpStream;

use serde::{Deserialize, Serialize};

const ADDR: &str = "127.0.0.1:48081";

#[derive(Serialize)]
struct RpcRequest {
    method: String,
    a: i32,
    b: i32,
}

#[derive(Debug, Deserialize)]
struct RpcResponse {
    result: i32,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut stream = TcpStream::connect(ADDR)?;
    let req = RpcRequest {
        method: "add".into(),
        a: 40,
        b: 2,
    };
    stream.write_all(&serde_json::to_vec(&req)?)?;
    let mut buf = [0u8; 1024];
    let n = stream.read(&mut buf)?;
    let resp: RpcResponse = serde_json::from_slice(&buf[..n])?;
    println!("RPC add(40,2) => {}", resp.result);
    Ok(())
}
