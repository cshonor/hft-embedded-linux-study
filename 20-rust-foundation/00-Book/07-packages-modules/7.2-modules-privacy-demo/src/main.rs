// bin crate root
use modules_privacy_demo::user_mod::User;

fn main() {
    modules_privacy_demo::eat_at_restaurant();

    let mut u = User::new(1, "abc");
    u.set_id(100); // ✅ 跨模块：通过 pub 方法改私有字段
    println!("user id = {}", u.get_id());

    // u.id = 100; // ❌ E0616：字段私有，跨模块不能直接改

    println!("ok: bin crate 调 lib 公开 API + User 封装");
}
