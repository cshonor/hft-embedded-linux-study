//! 19.5 声明宏 demo：`my_vec!`

/// 精简版 `vec!`：0..n 个表达式 → `Vec` + `push`
#[macro_export]
macro_rules! my_vec {
    () => {{
        Vec::new()
    }};
    ( $( $x:expr ),* $(,)? ) => {{
        let mut temp_vec = Vec::new();
        $(
            temp_vec.push($x);
        )*
        temp_vec
    }};
}

pub fn demo_decl_macro() {
    let v1: Vec<u32> = my_vec![1, 2, 3];
    let v2: Vec<&str> = my_vec!["a", "b"];
    let v3: Vec<i32> = my_vec![];

    println!("  my_vec![1,2,3] = {v1:?}");
    println!("  my_vec![\"a\",\"b\"] = {v2:?}");
    println!("  my_vec![] = {v3:?}");
}

pub fn demo_compile_error_notes() {
    println!("  【易错 1】macro_rules! 分支顺序");
    println!("    从上到下匹配；通用分支（如 catch-all）放最后");
    println!();
    println!("  【易错 2】重复语法");
    println!("    $( $x:expr ),*  — 0+ 个，逗号分隔");
    println!("    $( $x:expr ),+  — 1+ 个，不允许空");
    println!("    $( ... )?       — 0 或 1 个");
    println!();
    println!("  【易错 3】过程宏位置");
    println!("    不能写在 pancakes/decl_macro_demo 等业务 crate");
    println!("    → 独立 hello_macro_derive + proc-macro = true");
    println!();
    println!("  【易错 4】循环依赖");
    println!("    trait crate ← derive crate ← 使用方；勿互相 path 依赖成环");
    println!();
    println!("  【易错 5】宏报错难读");
    println!("    → cargo expand -p pancakes 查看展开代码");
    println!();
    println!("  【易错 6】宏 vs 函数");
    println!("    宏=编译期展开；函数=运行期调用；优先函数");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn my_vec_empty() {
        let v: Vec<i32> = my_vec![];
        assert!(v.is_empty());
    }

    #[test]
    fn my_vec_items() {
        let v = my_vec![1, 2, 3];
        assert_eq!(v, vec![1, 2, 3]);
    }

    #[test]
    fn my_vec_trailing_comma() {
        let v = my_vec![1, 2,];
        assert_eq!(v, vec![1, 2]);
    }
}
