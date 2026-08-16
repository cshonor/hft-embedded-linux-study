pub struct Breakfast {
    pub toast: String,
    seasonal_fruit: String,
}

impl Breakfast {
    pub fn summer(toast: &str) -> Breakfast {
        Breakfast {
            toast: String::from(toast),
            seasonal_fruit: String::from("peaches"),
        }
    }

    pub fn seasonal_fruit(&self) -> &str {
        &self.seasonal_fruit
    }
}

pub enum Appetizer {
    Soup,
    Salad,
}

pub fn fix_incorrect_order() {
    self::cook_order();
    super::serve_order();
}

fn cook_order() {}

/// 在子模块文件里演示：绝对 vs super 相对
pub fn demo_paths_from_back_of_house() {
    crate::front_of_house::hosting::add_to_waitlist();
    super::front_of_house::hosting::add_to_waitlist();
}
