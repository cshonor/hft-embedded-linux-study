# Item 20 · 案例与代码

← [Item 20 目录](./README.md)

### TLV 零拷贝困境

```rust
struct Tlv<'a> {
    value: &'a [u8],
}

struct NetworkServer<'a> {
    max_size: Option<Tlv<'a>>,
}

// 循环里
let data: Vec<u8> = read_packet();
server.max_size = Some(parse_tlv(&data)); // ❌ data does not live long enough
```

### 所有权改造

```rust
struct Tlv {
    value: Vec<u8>,
}

fn parse_tlv(input: &[u8]) -> Tlv {
    let len = /* ... */;
    Tlv {
        value: input[2..2 + len].to_vec(), // 显式分配，擦除 'a
    }
}

struct NetworkServer {
    max_size: Option<Tlv>, // ✅ 可长期持有
}
```

---
