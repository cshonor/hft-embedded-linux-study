// 6.2 match 控制流运算符 - 示例

#[derive(Debug)]
enum UsState {
    Alabama,
    Alaska,
}

#[derive(Debug)]
enum Coin {
    Penny,
    Nickel,
    Dime,
    Quarter(UsState),
}

fn value_in_cents(coin: Coin) -> u8 {
    match coin {
        Coin::Penny => {
            println!("Lucky penny!");
            1
        }
        Coin::Nickel => 5,
        Coin::Dime => 10,
        Coin::Quarter(state) => {
            println!("State quarter from {:?}!", state);
            25
        }
    }
}

fn plus_one(x: Option<i32>) -> Option<i32> {
    match x {
        None => None,
        Some(i) => Some(i + 1),
    }
}

fn add_fancy_hat() {
    println!("add_fancy_hat()");
}
fn remove_fancy_hat() {
    println!("remove_fancy_hat()");
}
fn move_player(num_spaces: u8) {
    println!("move_player({})", num_spaces);
}
fn reroll() {
    println!("reroll()");
}

fn main() {
    println!("=== 1) Coin + match ===");
    let c1 = Coin::Penny;
    let c2 = Coin::Quarter(UsState::Alaska);
    println!("value c1 = {}", value_in_cents(c1));
    println!("value c2 = {}", value_in_cents(c2));

    println!("\n=== 2) Option + match ===");
    let five = Some(5);
    let six = plus_one(five);
    let none = plus_one(None);
    println!("six = {:?}, none = {:?}", six, none);

    println!("\n=== 3) 通配分支：other 绑定值 ===");
    let dice_roll = 9;
    match dice_roll {
        3 => add_fancy_hat(),
        7 => remove_fancy_hat(),
        other => move_player(other),
    }

    println!("\n=== 4) 通配分支：_ 忽略值 ===");
    let dice_roll = 9;
    match dice_roll {
        3 => add_fancy_hat(),
        7 => remove_fancy_hat(),
        _ => reroll(),
    }

    println!("\n=== 5) _ => () 什么也不做 ===");
    let dice_roll = 9;
    match dice_roll {
        3 => add_fancy_hat(),
        7 => remove_fancy_hat(),
        _ => (),
    }
}

