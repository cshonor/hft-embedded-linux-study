use std::error::Error;
use std::fs::{self, File, OpenOptions};
use std::io::{self, ErrorKind, Read, Write};

const USERNAME_FILE: &str = "hello.txt";

fn main() -> Result<(), Box<dyn Error>> {
    let mode = std::env::args().nth(1).unwrap_or_else(|| "help".to_string());

    match mode.as_str() {
        "match_basic" => demo_match_basic()?,
        "nested" => demo_nested_match()?,
        "or_else" => demo_unwrap_or_else()?,
        "open_or_create" => {
            let mut f = open_or_create(USERNAME_FILE)?;
            writeln!(f, "username=rustacean")?;
            println!("wrote {USERNAME_FILE}");
        }
        "read_match" => {
            let username = read_username_from_file_match(USERNAME_FILE)?;
            println!("read_match: {username:?}");
        }
        "read_q" => {
            let username = read_username_from_file_q(USERNAME_FILE)?;
            println!("read_q: {username:?}");
        }
        "expect" => {
            let mut s = String::new();
            File::open("definitely_missing.txt")
                .expect("Failed to open definitely_missing.txt")
                .read_to_string(&mut s)
                .unwrap();
            println!("{s}");
        }
        "help" | _ => {
            eprintln!(
                "usage:\n\
                  cargo run -- match_basic\n\
                  cargo run -- nested\n\
                  cargo run -- or_else\n\
                  cargo run -- open_or_create\n\
                  cargo run -- read_match\n\
                  cargo run -- read_q\n\
                  cargo run -- expect"
            );
        }
    }

    Ok(())
}

/// §二 单层 match
fn demo_match_basic() -> Result<(), Box<dyn Error>> {
    let res = File::open(USERNAME_FILE);
    let _file = match res {
        Ok(f) => {
            println!("match_basic: 打开成功");
            f
        }
        Err(e) => panic!("打开失败: {e:?}"),
    };
    Ok(())
}

/// §三 嵌套 match + ErrorKind（书本经典）
fn demo_nested_match() -> Result<(), Box<dyn Error>> {
    let file = match File::open(USERNAME_FILE) {
        Ok(f) => f,
        Err(e) => match e.kind() {
            ErrorKind::NotFound => match File::create(USERNAME_FILE) {
                Ok(new_f) => {
                    println!("nested: NotFound → 已创建 {USERNAME_FILE}");
                    new_f
                }
                Err(create_err) => panic!("创建失败: {create_err:?}"),
            },
            other => panic!("未知打开错误: {other:?}"),
        },
    };
    drop(file);
    Ok(())
}

/// §四 unwrap_or_else 简化
fn demo_unwrap_or_else() -> Result<(), Box<dyn Error>> {
    let file = File::open(USERNAME_FILE).unwrap_or_else(|e| {
        if e.kind() == ErrorKind::NotFound {
            println!("or_else: NotFound → File::create");
            File::create(USERNAME_FILE).unwrap()
        } else {
            panic!("打开异常: {e:?}");
        }
    });
    drop(file);
    Ok(())
}

fn open_or_create(path: &str) -> Result<File, io::Error> {
    match File::open(path) {
        Ok(file) => Ok(file),
        Err(error) => match error.kind() {
            io::ErrorKind::NotFound => File::create(path),
            other => Err(io::Error::new(other, format!("Problem opening {path}"))),
        },
    }
}

fn read_username_from_file_match(path: &str) -> Result<String, io::Error> {
    let mut f = match File::open(path) {
        Ok(file) => file,
        Err(e) => return Err(e),
    };

    let mut s = String::new();
    match f.read_to_string(&mut s) {
        Ok(_) => Ok(s),
        Err(e) => Err(e),
    }
}

fn read_username_from_file_q(path: &str) -> Result<String, io::Error> {
    let mut s = String::new();
    File::open(path)?.read_to_string(&mut s)?;
    Ok(s)
}

#[allow(dead_code)]
fn read_username_from_file_std(path: &str) -> Result<String, io::Error> {
    fs::read_to_string(path)
}

#[allow(dead_code)]
fn append_line(path: &str, line: &str) -> Result<(), io::Error> {
    let mut f = OpenOptions::new().create(true).append(true).open(path)?;
    writeln!(f, "{line}")?;
    Ok(())
}
