//! RFR ch02 §07：孤儿规则 · Coverage · NewType 演示。

use std::fmt;
use std::ops::Deref;

// --- 合法 1：本地 trait，外部类型 ---

trait MySerialize {
    fn tag(&self) -> &'static str;
}

impl MySerialize for Vec<u8> {
    fn tag(&self) -> &'static str {
        "vec-u8"
    }
}

// --- 合法 2：本地类型，外部 trait（NewType）---

pub struct WrapperVec(Vec<u8>);

impl WrapperVec {
    pub fn new(inner: Vec<u8>) -> Self {
        Self(inner)
    }
}

impl Deref for WrapperVec {
    type Target = Vec<u8>;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl fmt::Display for WrapperVec {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "WrapperVec(len={})", self.0.len())
    }
}

// 双外部非法示例（编译失败，勿取消注释）：
// impl fmt::Display for Vec<u8> {
//     fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
//         write!(f, "{:?}", self)
//     }
// }

// --- 合法 3：Coverage — 本地类型出现在 From 泛型参数 ---

struct MyData;

impl From<MyData> for Vec<u8> {
    fn from(_: MyData) -> Self {
        vec![9, 8, 7]
    }
}

// --- Blanket impl 示意（本地 trait）---

trait MyLocalTrait {
    fn marker(&self) -> &'static str {
        "my-local"
    }
}

impl<T: fmt::Debug> MyLocalTrait for T {}

fn main() {
    let v: Vec<u8> = vec![1, 2, 3];
    println!("MySerialize tag: {}", v.tag());

    let wrapped = WrapperVec::new(vec![4, 5, 6]);
    println!("Display: {wrapped}");
    println!("Deref len: {}", wrapped.len());

    let from_local: Vec<u8> = MyData.into();
    println!("From<MyData>: {:?}", from_local);

    println!("blanket on Vec: {}", v.marker());
}
