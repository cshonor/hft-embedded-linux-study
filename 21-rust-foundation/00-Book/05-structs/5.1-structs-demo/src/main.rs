// 5.1 定义并实例化结构体 - 示例

#[derive(Debug, Clone)]
struct User {
    active: bool,
    username: String,
    email: String,
    sign_in_count: u64,
}

fn build_user(email: String, username: String) -> User {
    // 字段初始化简写：变量名与字段名相同
    User {
        email,
        username,
        active: true,
        sign_in_count: 1,
    }
}

// 元组结构体：有名字但字段无名字
#[derive(Debug)]
struct Color(i32, i32, i32);

#[derive(Debug)]
struct Point(i32, i32, i32);

// 类单元结构体：没有字段
#[derive(Debug)]
struct AlwaysEqual;

fn main() {
    println!("=== 1) 实例化 + 访问字段 ===");
    let user1 = User {
        email: String::from("someone@example.com"),
        username: String::from("someusername123"),
        active: true,
        sign_in_count: 1,
    };
    println!("user1.email = {}", user1.email);

    println!("\n=== 2) 修改字段（实例必须 mut）===");
    let mut user2 = build_user(String::from("a@ex.com"), String::from("alice"));
    user2.email = String::from("alice@ex.com");
    println!("user2 = {:?}", user2);

    println!("\n=== 3) ..clone() 更新（原实例保留，非 Copy 字段被 clone）===");
    let user3 = User {
        email: String::from("cloned@example.com"),
        ..user2.clone()
    };
    println!("user3 = {:?}", user3);
    println!("user2 still full after ..clone(): {:?}", user2);

    println!("\n=== 4) 结构体更新语法（部分 move）===");
    let user4 = User {
        email: String::from("another@example.com"),
        // .. 会把剩余字段从 user2 移动/拷贝：
        // - username: String → move
        // - active, sign_in_count → Copy
        ..user2
    };
    println!("user4 = {:?}", user4);
    println!("user2.active still ok: {}", user2.active);
    // println!("{:?}", user2);              // ❌ partial moved
    // println!("{}", user2.username);       // ❌ moved

    println!("\n=== 5) 全 Copy 结构体 .. 后原实例完好 ===");
    #[derive(Debug, Copy, Clone)]
    struct Info {
        a: u32,
        b: bool,
    }
    let i1 = Info { a: 1, b: true };
    let i2 = Info { a: 99, ..i1 };
    println!("i1 = {:?}, i2 = {:?}", i1, i2);

    println!("\n=== 6) 元组结构体 ===");
    let black = Color(0, 0, 0);
    let origin = Point(0, 0, 0);
    println!("black.0 = {}, origin.1 = {}", black.0, origin.1);
    println!("black = {:?}, origin = {:?}", black, origin);

    println!("\n=== 7) 类单元结构体 ===");
    let subject = AlwaysEqual;
    println!("subject = {:?}", subject);
}

