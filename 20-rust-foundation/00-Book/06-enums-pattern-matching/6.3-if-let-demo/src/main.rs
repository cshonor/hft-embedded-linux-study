// 6.3 if let 简单控制流 - 示例

#[derive(Debug)]
enum UsState {
    Alaska,
}

#[derive(Debug)]
enum Coin {
    Penny,
    Quarter(UsState),
}

fn main() {
    println!("=== 1) match 只关心 Some(3) ===");
    let some_u8_value = Some(0u8);
    match some_u8_value {
        Some(3) => println!("three"),
        _ => (),
    }

    println!("\n=== 2) if let 更简洁 ===");
    let some_u8_value = Some(3u8);
    if let Some(3) = some_u8_value {
        println!("three");
    }

    println!("\n=== 3) if let + else ===");
    let mut count = 0;

    let coin = Coin::Quarter(UsState::Alaska);
    if let Coin::Quarter(state) = coin {
        println!("State quarter from {:?}!", state);
    } else {
        count += 1;
    }

    let coin = Coin::Penny;
    if let Coin::Quarter(state) = coin {
        println!("State quarter from {:?}!", state);
    } else {
        count += 1;
    }

    println!("non-quarter count = {}", count);
}

