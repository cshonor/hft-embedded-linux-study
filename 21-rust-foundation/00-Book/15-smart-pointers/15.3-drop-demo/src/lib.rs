//! 15.3 Drop / RAII demo

use std::mem::{self, ManuallyDrop};
use std::sync::Mutex;

/// 带日志的 RAII 标签，观察 drop 顺序。
#[derive(Debug)]
pub struct Tag(pub &'static str);

impl Drop for Tag {
    fn drop(&mut self) {
        println!("  drop Tag({})", self.0);
    }
}

/// 书版 CustomSmartPointer
pub struct CustomSmartPointer {
    pub data: String,
}

impl Drop for CustomSmartPointer {
    fn drop(&mut self) {
        println!("  Dropping CustomSmartPointer `{}`", self.data);
    }
}

struct Inner;
struct Outer(Inner);

impl Drop for Inner {
    fn drop(&mut self) {
        println!("  drop Inner");
    }
}

impl Drop for Outer {
    fn drop(&mut self) {
        println!("  drop Outer");
    }
}

struct CountDrop(u32);

impl Drop for CountDrop {
    fn drop(&mut self) {
        println!("  drop E({})", self.0);
    }
}

struct Resource;

impl Drop for Resource {
    fn drop(&mut self) {
        println!("  Resource freed");
    }
}

/// 带 Drop 日志的字段，观察「自定义 drop 之后字段才 drop」
struct FieldTag(&'static str);

impl Drop for FieldTag {
    fn drop(&mut self) {
        println!("  字段 {} 内存回收", self.0);
    }
}

/// §二 模拟 FileHandle：外部资源 + 内存字段
pub struct FileHandle {
    pub fd: i32,
    buf: FieldTag,
}

impl Drop for FileHandle {
    fn drop(&mut self) {
        println!("  手动执行：关闭 fd = {}（外部资源）", self.fd);
    }
}

pub fn demo_custom_drop_then_fields() {
    let _fh = FileHandle {
        fd: 10,
        buf: FieldTag("buf/Vec"),
    };
    println!("  FileHandle 创建完成，即将出作用域");
}

/// §15.3.1 LIFO：Tag("a") / Tag("b")，验证 drop b → drop a
pub fn demo_tag_lifo() {
    println!("  创建 a，再创建 b…");
    let _a = Tag("a");
    let _b = Tag("b");
    println!("  即将出作用域（预期先 drop b，再 drop a）");
}

/// §三 作用域结束自动 drop + §五 LIFO 顺序
pub fn demo_scope_and_order() {
    println!("  --- 进入内层作用域 ---");
    {
        let _f = Tag("block");
    }
    println!("  --- 内层作用域结束 ---");

    let c = CustomSmartPointer {
        data: String::from("my stuff"),
    };
    let d = CustomSmartPointer {
        data: String::from("other stuff"),
    };
    println!("  CustomSmartPointers created（离开函数时先 d 后 c）");
    let _ = (c, d);
}

/// §三 mem::drop 提前释放
pub fn demo_mem_drop_early() {
    let c = CustomSmartPointer {
        data: String::from("some data"),
    };
    println!("  CustomSmartPointer created");
    mem::drop(c);
    println!("  mem::drop 之后（c 已不可用）");
}

/// §15.3.1 嵌套：Outer → Inner
pub fn demo_nested_drop() {
    let _o = Outer(Inner);
    println!("  Outer(Inner) 即将出作用域");
}

/// §15.3.1 Vec 元素正序 drop
pub fn demo_vec_drop_order() {
    let v = vec![
        CountDrop(1),
        CountDrop(2),
        CountDrop(3),
    ];
    println!("  Vec 含 3 元素，即将 drop");
    drop(v);
}

/// §15.3.0 OBRM：借用不触发 Drop，只有所有者出作用域才释放
struct LoudDrop(i32);

impl Drop for LoudDrop {
    fn drop(&mut self) {
        println!("  OBRM: LoudDrop({}) released", self.0);
    }
}

pub fn demo_obrm_borrow_vs_owner() {
    println!("  --- 进入内层作用域 ---");
    {
        let b = Box::new(LoudDrop(100));
        let r = &*b;
        println!("  b 是所有者；&*b 只是借用，r.0 = {}", r.0);
        println!("  r 出作用域：不应打印 released");
    }
    println!("  --- 内层结束（上方应已打印 released）---");
}

/// §四 MutexGuard RAII 解锁（Drop 无 println，仅演示编译通过）
pub fn demo_mutex_guard_drop() {
    let m = Mutex::new(0_i32);
    {
        let mut guard = m.lock().unwrap();
        *guard += 1;
        println!("  持锁中: value = {}", *guard);
    }
    let after = m.lock().unwrap();
    println!("  锁已释放，可再次 lock: value = {}", *after);
}

/// §15.3.1 ManuallyDrop
pub fn demo_manually_drop() {
    let mut slot = ManuallyDrop::new(Resource);
    println!("  ManuallyDrop 包裹 Resource（尚未 drop）");
    let taken = unsafe { ManuallyDrop::take(&mut slot) };
    drop(taken);
    println!("  手动 take + drop 完成");
}

// ── 15.3.2 网络 Socket RAII ─────────────────────────────

/// 教学用：模拟持有裸 fd 的 TCP 封装（实际项目调 `libc::close`）
pub struct TcpSocket {
    fd: i32,
    peer_addr: String,
}

impl TcpSocket {
    pub fn new(fd: i32, addr: &str) -> Self {
        Self {
            fd,
            peer_addr: addr.to_string(),
        }
    }

    pub fn fd(&self) -> i32 {
        self.fd
    }

    pub fn close(&mut self) {
        if self.fd != -1 {
            println!(
                "  关闭 Socket fd: {}, 对端: {}",
                self.fd, self.peer_addr
            );
            self.fd = -1;
        }
    }
}

impl Drop for TcpSocket {
    fn drop(&mut self) {
        self.close();
    }
}

/// 模拟标准库 `TcpStream`：内部已实现 Drop
struct OwnedStream {
    fd: i32,
}

impl OwnedStream {
    fn new(fd: i32) -> Self {
        Self { fd }
    }
}

impl Drop for OwnedStream {
    fn drop(&mut self) {
        if self.fd != -1 {
            println!("  OwnedStream（类 TcpStream）关闭 fd: {}", self.fd);
            self.fd = -1;
        }
    }
}

/// 业务 Conn：只嵌套 socket + 缓冲区，无需手写 Drop
pub struct Conn {
    socket: OwnedStream,
    recv_buf: Vec<u8>,
    send_buf: Vec<u8>,
    active: bool,
}

impl Conn {
    pub fn new(fd: i32) -> Self {
        Self {
            socket: OwnedStream::new(fd),
            recv_buf: Vec::new(),
            send_buf: Vec::new(),
            active: true,
        }
    }
}

fn handle_conn(sock: TcpSocket) {
    println!("  handle_conn 持有着 fd = {}", sock.fd());
}

/// §1 出作用域自动 close
pub fn demo_socket_scope() {
    let _sock = TcpSocket::new(1001, "127.0.0.1:8080");
    println!("  连接正常使用中");
}

/// §2 Conn 默认 drop → 内层 OwnedStream close
pub fn demo_conn_default_drop() {
    let _conn = Conn::new(2002);
    println!("  Conn 使用中（无手写 Drop）");
}

/// §3 move：原变量不在此 close，handle_conn 返回时才 close
pub fn demo_socket_move() {
    let sock = TcpSocket::new(1001, "127.0.0.1:8080");
    handle_conn(sock);
    println!("  handle_conn 返回后（连接已在函数内关闭）");
}

/// §4 提前 close / mem::drop
pub fn demo_socket_early_close() {
    let mut sock = TcpSocket::new(3003, "10.0.0.1:443");
    println!("  主动 close()");
    sock.close();
    println!("  出作用域时 drop 不会重复 close（fd 已是 -1）");

    let sock2 = TcpSocket::new(4004, "10.0.0.2:443");
    mem::drop(sock2);
    println!("  mem::drop 提前销毁 sock2");
}

pub fn demo_socket_all() {
    println!("--- §1 出作用域 auto close ---");
    demo_socket_scope();

    println!("\n--- §2 Conn 默认 drop 关内层 socket ---");
    demo_conn_default_drop();

    println!("\n--- §3 move 语义 ---");
    demo_socket_move();

    println!("\n--- §4 提前 close / mem::drop ---");
    demo_socket_early_close();
}

#[cfg(test)]
mod socket_tests {
    use super::TcpSocket;

    #[test]
    fn socket_close_once() {
        let mut s = TcpSocket::new(42, "test");
        s.close();
        assert_eq!(s.fd(), -1);
        drop(s); // drop 不应 panic，也不重复 close
    }
}

#[cfg(test)]
mod tests {
    use std::sync::Mutex as StdMutex;

    static DROP_LOG: StdMutex<Vec<&'static str>> = StdMutex::new(Vec::new());

    struct LogOnDrop(&'static str);

    impl Drop for LogOnDrop {
        fn drop(&mut self) {
            DROP_LOG.lock().unwrap().push(self.0);
        }
    }

    #[test]
    fn drop_order_lifo() {
        {
            let mut log = DROP_LOG.lock().unwrap();
            log.clear();
        }
        {
            let _a = LogOnDrop("a");
            let _b = LogOnDrop("b");
        }
        let log = DROP_LOG.lock().unwrap();
        assert_eq!(*log, vec!["b", "a"]);
    }

    #[test]
    fn vec_drop_reverse() {
        {
            let mut log = DROP_LOG.lock().unwrap();
            log.clear();
        }
        struct E(u32);
        impl Drop for E {
            fn drop(&mut self) {
                DROP_LOG.lock().unwrap().push(match self.0 {
                    1 => "E1",
                    2 => "E2",
                    3 => "E3",
                    _ => "?",
                });
            }
        }
        drop(vec![E(1), E(2), E(3)]);
        let log = DROP_LOG.lock().unwrap();
        assert_eq!(*log, vec!["E1", "E2", "E3"]);
    }
}

// 编译失败示例（文档用，勿取消注释）：
// fn explicit_drop_disallowed() {
//     let mut f = Tag("x");
//     f.drop(); // error: explicit call to destructor `drop` is disallowed
// }
