//! Item 22: 最小可见性 — pub struct 字段须逐一 pub；pub(crate) 内部共享

pub mod public {
    #[derive(Debug, Default)]
    pub struct AStruct {
        pub name: String,
        count: i32,
    }

    impl AStruct {
        pub fn new(name: impl Into<String>) -> Self {
            Self {
                name: name.into(),
                count: 0,
            }
        }

        pub fn id(&self) -> String {
            format!("{}#{}", self.name, self.count)
        }

        pub(crate) fn bump(&mut self) {
            self.count += 1;
        }
    }
}

mod internal {
    use crate::public::AStruct;

    pub(crate) fn prepare() -> AStruct {
        let mut s = AStruct::new("demo");
        s.bump();
        s
    }
}

pub fn demo_id() -> String {
    internal::prepare().id()
}
