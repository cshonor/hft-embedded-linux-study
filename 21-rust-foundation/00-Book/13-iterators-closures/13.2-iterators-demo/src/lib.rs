//! 13.2 迭代器 demo — 惰性 · next · iter 三件套 · Counter · impl/dyn 返回

#[derive(PartialEq, Debug, Clone)]
pub struct Shoe {
    pub size: u32,
    pub style: String,
}

pub fn shoes_in_my_size(shoes: Vec<Shoe>, shoe_size: u32) -> Vec<Shoe> {
    shoes
        .into_iter()
        .filter(|s| s.size == shoe_size)
        .collect()
}

// ── 书版 Counter：产出 1..=5 ─────────────────────────
pub struct Counter {
    count: u32,
}

impl Counter {
    pub fn new() -> Self {
        Self { count: 0 }
    }
}

impl Iterator for Counter {
    type Item = u32;

    fn next(&mut self) -> Option<Self::Item> {
        self.count += 1;
        if self.count < 6 {
            Some(self.count)
        } else {
            None
        }
    }
}

// ── 13.2.1 CounterRange：curr..max ───────────────────
pub struct CounterRange {
    pub curr: usize,
    pub max: usize,
}

impl Iterator for CounterRange {
    type Item = usize;

    fn next(&mut self) -> Option<Self::Item> {
        if self.curr < self.max {
            let res = self.curr;
            self.curr += 1;
            Some(res)
        } else {
            None
        }
    }
}

/// §一 惰性：collect 前不运算（无日志，供 test）
pub fn lazy_map_filter(arr: [i32; 3]) -> Vec<i32> {
    let iter = arr.iter().map(|&x| x * 2).filter(|&v| v > 2);
    iter.collect()
}

// ── 13.2.4 惰性分步演示（带 println）──────────────────

pub fn demo_lazy_pipeline_only() {
    let v = vec![1, 2, 3, 4, 5];
    let _pipeline = v.iter().map(|x| x * 2).filter(|&x| x > 5);
    println!("  流水线搭建完成，还没有开始加工！（map/filter 未执行）");
}

pub fn demo_lazy_map_collect() {
    let v = vec![1, 2, 3];
    let iter = v.iter().map(|x| {
        println!("  正在处理元素: {}", x);
        x * 2
    });
    println!("  ===== 构建完迭代器，还没消费 =====");
    let result: Vec<_> = iter.collect();
    println!("  最终结果: {:?}", result);
}

pub fn demo_lazy_map_filter_chain() {
    let v = vec![1, 2, 3, 4, 5];
    let iter = v.iter().map(|x| {
        println!("  map 处理: {}", x);
        x * 2
    }).filter(|&x| {
        println!("  filter 判断: {}", x);
        x > 5
    });
    println!("  ===== 规则组装完成，未执行 =====");
    let res: Vec<_> = iter.collect();
    println!("  筛选结果: {:?}", res);
}

pub fn demo_lazy_skip_take() {
    let v = vec![10, 20, 30, 40];

    let iter = v.iter().skip(2);
    println!("  skip(2) 组装完成，未执行");
    let res: Vec<_> = iter.collect();
    println!("  skip 结果: {:?}", res);

    let iter = v.iter().take(2);
    println!("  take(2) 组装完成，未执行");
    let res: Vec<_> = iter.collect();
    println!("  take 结果: {:?}", res);
}

pub fn demo_lazy_consumers() {
    let v = vec![1, 2, 3];

    let iter = v.iter().map(|x| x * 2);
    println!("  sum: 组装完成");
    let total: i32 = iter.sum();
    println!("  sum 总和: {}", total);

    let iter = v.iter().map(|x| x * 2);
    println!("  for: 开始循环（触发执行）");
    for num in iter {
        println!("    元素: {}", num);
    }

    let v2 = vec![1, 2, 3, 4, 5];
    let iter = v2.iter().filter(|&&x| x % 2 == 0);
    println!("  count: 组装完成");
    let cnt = iter.count();
    println!("  偶数个数: {}", cnt);

    demo_lazy_while_let();
}

/// map + filter 链，闭包内 println，供三种消费器对比
fn lazy_traced(v: &Vec<i32>) -> impl Iterator<Item = i32> + '_ {
    v.iter().map(|x| {
        println!("    map: {}", x);
        x * 2
    }).filter(|&x| {
        println!("    filter: {}", x);
        x > 5
    })
}

pub fn demo_lazy_while_let() {
    let v = vec![1, 2, 3, 4, 5];
    let mut iter = v.iter().map(|x| x * 2).filter(|&x| x > 5);
    println!("  while let: 流水线搭好，手动 next()");
    while let Some(x) = iter.next() {
        println!("    拿到元素: {}", x);
    }
}

pub fn demo_lazy_three_consumers() {
    let v = vec![1, 2, 3, 4, 5];

    println!("  A) while let — 每轮显式 next()");
    let mut iter = lazy_traced(&v);
    while let Some(x) = iter.next() {
        println!("      while let 拿到: {}", x);
    }

    println!("  B) for — 语法糖，内部反复 next()");
    for x in lazy_traced(&v) {
        println!("      for 拿到: {}", x);
    }

    println!("  C) collect — 内部收齐，只出 Vec");
    let res: Vec<_> = lazy_traced(&v).collect();
    println!("      collect 成品: {:?}", res);
}

pub fn demo_lazy_consume_once() {
    let v = vec![1, 2, 3];
    let mut iter = v.iter().map(|x| x * 2);

    // 第一次：&mut iter 收集，迭代器本体仍在（但已耗尽）
    let res1: Vec<_> = (&mut iter).collect();
    println!("  第一次 collect: {:?}", res1);

    let res2: Vec<_> = iter.collect();
    println!("  第二次 collect: {:?}（已耗尽）", res2);
}

pub fn demo_lazy_call_chain() {
    let v = vec![1, 2, 3];

    let iter = v.iter().map(|x| {
        println!("  【map】处理元素: {}", x);
        x * 2
    }).filter(|&x| {
        println!("  【filter】判断元素: {}", x);
        x > 3
    });

    println!("  === 所有适配器包装完毕，等待消费 ===");
    let result: Vec<_> = iter.collect();
    println!("  最终结果: {:?}", result);
}

pub fn demo_lazy_all() {
    println!("--- §0 只搭流水线（无消费器）---");
    demo_lazy_pipeline_only();

    println!("\n--- §1 map + collect ---");
    demo_lazy_map_collect();

    println!("\n--- §2 map + filter 链 ---");
    demo_lazy_map_filter_chain();

    println!("\n--- §3 skip / take ---");
    demo_lazy_skip_take();

    println!("\n--- §4 sum / for / count / while let ---");
    demo_lazy_consumers();

    println!("\n--- §5 三种消费器同链对比 ---");
    demo_lazy_three_consumers();

    println!("\n--- §6 消费后不能复用 ---");
    demo_lazy_consume_once();
}

/// §五 返回 impl Iterator
pub fn get_iter() -> impl Iterator<Item = i32> {
    vec![1, 2, 3].into_iter().map(|x| x * 2)
}

/// 13.2.6：三种结构体顺序使用（每步 `{}` 释放借用，避免 iter+iter_mut 冲突）
pub fn demo_iter_structs() {
    let mut v = vec![10, 20, 30];

    {
        let it1 = v.iter();
        let cnt = it1.count();
        println!("  Iter<'_, i32> count = {}，v 仍 {:?}", cnt, v);
    }

    {
        let it2 = v.iter_mut();
        for x in it2 {
            *x += 5;
        }
    }
    println!("  IterMut 后 v = {:?}", v);

    let v_ref = vec![1, 2, 3];
    let r1: Vec<&i32> = v_ref.iter().collect();
    println!("  iter+collect → {:?}（v_ref 仍 {:?}）", r1, v_ref);

    let r2: Vec<i32> = v.into_iter().collect();
    println!("  IntoIter+collect → {:?}", r2);
    // v 已 move

    // ❌ 编译失败示例（勿取消注释）：
    // let mut v = vec![1, 2, 3];
    // let _a = v.iter();
    // let _b = v.iter_mut();
}

/// §13.2.5 三种 iter 演示（作用域避免借用冲突）
pub fn demo_iter_kinds() {
    let mut v = vec![10, 20, 30];

    {
        let val: Vec<&i32> = v.iter().collect();
        println!("  iter collect: {:?}", val);
        println!("  v 仍可用: {:?}", v);
    }

    {
        let refs: Vec<&mut i32> = v.iter_mut().collect();
        for x in refs {
            *x *= 2;
        }
    }
    println!("  iter_mut ×2 后: {:?}", v);

    let owned: Vec<i32> = v.into_iter().collect();
    println!("  into_iter owned: {:?}", owned);

    let a = vec![1, 2];
    let b = vec![10, 20];
    let z: Vec<(&i32, &i32)> = a.iter().zip(b.iter()).collect();
    println!("  zip iter: {:?}（a、b 仍可用 {:?} {:?}）", z, a, b);

    let mut m = vec![1, 2];
    for x in &mut m {
        *x += 1;
    }
    println!("  for x in &mut m → {:?}", m);

    let r = vec![5, 6];
    let mut sum = 0;
    for x in &r {
        sum += x;
    }
    println!("  for x in &r → sum={}，r 仍 {:?}", sum, r);
}

/// §五 运行期两种迭代器 → Box<dyn>
pub fn get_dyn(flag: bool) -> Box<dyn Iterator<Item = i32>> {
    if flag {
        Box::new(vec![1, 2].into_iter())
    } else {
        Box::new((0..5).into_iter())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lazy_consume_once_empty() {
        let v = vec![1, 2, 3];
        let mut iter = v.iter().map(|x| x * 2);
        let first: Vec<_> = (&mut iter).collect();
        let second: Vec<_> = iter.collect();
        assert_eq!(first, vec![2, 4, 6]);
        assert!(second.is_empty());
    }

    #[test]
    fn lazy_until_collect() {
        let out = lazy_map_filter([1, 2, 3]);
        assert_eq!(out, vec![4, 6]);
    }

    #[test]
    fn while_let_same_as_for_and_collect() {
        let v = vec![1, 2, 3, 4, 5];
        fn pipeline(v: &Vec<i32>) -> impl Iterator<Item = i32> + '_ {
            v.iter().map(|x| x * 2).filter(|&x| x > 5)
        }

        let mut iter = pipeline(&v);
        let mut manual = Vec::new();
        while let Some(x) = iter.next() {
            manual.push(x);
        }

        let from_for: Vec<_> = pipeline(&v).collect();
        let from_collect: Vec<_> = pipeline(&v).collect();

        assert_eq!(manual, vec![6, 8, 10]);
        assert_eq!(from_for, manual);
        assert_eq!(from_collect, manual);
    }

    #[test]
    fn iterator_demonstration() {
        let v1 = vec![1, 2, 3];
        let mut v1_iter = v1.iter();

        assert_eq!(v1_iter.next(), Some(&1));
        assert_eq!(v1_iter.next(), Some(&2));
        assert_eq!(v1_iter.next(), Some(&3));
        assert_eq!(v1_iter.next(), None);
        assert_eq!(v1_iter.next(), None);
    }

    #[test]
    fn iter_mut_bumps() {
        let mut arr = vec![10, 20, 30];
        for v in arr.iter_mut() {
            *v += 1;
        }
        assert_eq!(arr, vec![11, 21, 31]);
    }

    #[test]
    fn iter_structs_sequential_ok() {
        let mut v = vec![10, 20, 30];
        {
            let _it1 = v.iter();
            assert_eq!(_it1.count(), 3);
        }
        {
            let mut it2 = v.iter_mut();
            it2.next().map(|x| *x *= 2);
        }
        assert_eq!(v[0], 20);
        let out: Vec<i32> = v.into_iter().collect();
        assert_eq!(out, vec![20, 20, 30]);
    }

    #[test]
    fn iter_kinds_iter_collect_keeps_vec() {
        let mut v = vec![10, 20, 30];
        {
            let refs: Vec<&i32> = v.iter().collect();
            assert_eq!(refs, vec![&10, &20, &30]);
        }
        v.iter_mut().for_each(|x| *x += 1);
        assert_eq!(v, vec![11, 21, 31]);
    }

    #[test]
    fn iter_kinds_iter_mut_exclusive_scope() {
        let mut v = vec![1, 2, 3];
        {
            let refs: Vec<&mut i32> = v.iter_mut().collect();
            for x in refs {
                *x *= 2;
            }
        }
        assert_eq!(v, vec![2, 4, 6]);
    }

    #[test]
    fn iter_kinds_for_ref_and_for_mut() {
        let v = vec![1, 2];
        let mut sum = 0i32;
        for x in &v {
            sum += x;
        }
        assert_eq!(sum, 3);

        let mut m = vec![1, 2];
        for x in &mut m {
            *x += 10;
        }
        assert_eq!(m, vec![11, 12]);
    }

    #[test]
    fn iter_kinds_zip_collect_borrows() {
        let a = vec![1, 2];
        let b = vec![10, 20];
        let z: Vec<(&i32, &i32)> = a.iter().zip(b.iter()).collect();
        assert_eq!(z, vec![(&1, &10), (&2, &20)]);
        assert_eq!(a.len(), 2);
        assert_eq!(b.len(), 2);
    }

    #[test]
    fn into_iter_moves() {
        let arr = vec![1, 2, 3];
        let owned: Vec<i32> = arr.into_iter().collect();
        assert_eq!(owned, vec![1, 2, 3]);
    }

    #[test]
    fn iterator_sum() {
        let v1 = vec![1, 2, 3];
        let total: i32 = v1.iter().sum();
        assert_eq!(total, 6);
    }

    #[test]
    fn iterator_map_collect() {
        let v1: Vec<i32> = vec![1, 2, 3];
        let v2: Vec<_> = v1.iter().map(|x| x + 1).collect();
        assert_eq!(v2, vec![2, 3, 4]);
    }

    #[test]
    fn filters_by_size() {
        let shoes = vec![
            Shoe {
                size: 10,
                style: String::from("sneaker"),
            },
            Shoe {
                size: 13,
                style: String::from("sandal"),
            },
            Shoe {
                size: 10,
                style: String::from("boot"),
            },
        ];

        let in_my_size = shoes_in_my_size(shoes, 10);

        assert_eq!(
            in_my_size,
            vec![
                Shoe {
                    size: 10,
                    style: String::from("sneaker")
                },
                Shoe {
                    size: 10,
                    style: String::from("boot")
                },
            ]
        );
    }

    #[test]
    fn calling_next_directly() {
        let mut counter = Counter::new();

        assert_eq!(counter.next(), Some(1));
        assert_eq!(counter.next(), Some(2));
        assert_eq!(counter.next(), Some(3));
        assert_eq!(counter.next(), Some(4));
        assert_eq!(counter.next(), Some(5));
        assert_eq!(counter.next(), None);
    }

    #[test]
    fn counter_range_sum() {
        let sum: usize = CounterRange { curr: 1, max: 5 }
            .map(|x| x * 2)
            .sum();
        assert_eq!(sum, 20);
    }

    #[test]
    fn using_other_iterator_trait_methods() {
        let sum: u32 = Counter::new()
            .zip(Counter::new().skip(1))
            .map(|(a, b)| a * b)
            .filter(|x| x % 3 == 0)
            .sum();

        assert_eq!(18, sum);
    }

    #[test]
    fn impl_and_dyn_iterators() {
        let impl_vals: Vec<i32> = get_iter().collect();
        assert_eq!(impl_vals, vec![2, 4, 6]);

        let dyn_a: Vec<i32> = get_dyn(true).collect();
        let dyn_b: Vec<i32> = get_dyn(false).collect();
        assert_eq!(dyn_a, vec![1, 2]);
        assert_eq!(dyn_b, vec![0, 1, 2, 3, 4]);
    }
}
