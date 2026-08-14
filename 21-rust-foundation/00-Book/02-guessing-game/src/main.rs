// 引入所需模块
use rand::Rng;           // 随机数生成器 trait
use std::cmp::Ordering;  // 用于比较的枚举 (Less/Equal/Greater)
use std::io;             // 标准输入输出

fn main() {
    println!("猜数字游戏!");

    // 生成 1-100 之间的秘密数字 (包含 1 和 100)
    // thread_rng() 获取线程本地随机数生成器
    let secret_number = rand::thread_rng().gen_range(1..=100);

    // 无限循环，直到猜对 break
    loop {
        println!("请输入你的猜测 (1-100):");

        // 创建可变字符串存储用户输入
        // mut 表示可变，String::new() 创建空字符串
        let mut guess = String::new();

        // 从标准输入读取一行，追加到 guess
        // &mut guess 是可变借用
        // expect 在失败时打印信息并 panic
        io::stdin()
            .read_line(&mut guess)
            .expect("读取行失败");

        // 将字符串转换为 u32 数字
        // trim() 去除首尾空白（包括换行符）
        // parse() 返回 Result<u32, ParseIntError>
        // match 处理 Ok/Err：成功取 num，失败则提示并 continue 下一轮
        let guess: u32 = match guess.trim().parse() {
            Ok(num) => num,
            Err(_) => {
                println!("请输入有效的数字!");
                continue;
            }
        };

        println!("你猜的是: {}", guess);

        // 比较猜测值与秘密数字
        // cmp 返回 Ordering 枚举
        match guess.cmp(&secret_number) {
            Ordering::Less => println!("太小了!"),      // 猜小了
            Ordering::Greater => println!("太大了!"),   // 猜大了
            Ordering::Equal => {
                println!("恭喜，你赢了!");
                break;  // 猜对，退出循环
            }
        }
    }
}

