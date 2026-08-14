pub struct User {
    id: u64,
    name: String,
}

impl User {
    pub fn new(id: u64, name: impl Into<String>) -> Self {
        Self {
            id,
            name: name.into(),
        }
    }

    pub fn set_id(&mut self, val: u64) {
        self.id = val;
    }

    pub fn get_id(&self) -> u64 {
        self.id
    }
}

/// 同模块内：私有字段可直接读写
#[allow(dead_code)]
fn same_module_demo() {
    let mut u = User {
        id: 1,
        name: "abc".into(),
    };
    u.id = 999;
    u.name = "新名字".into();
}
