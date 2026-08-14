pub fn run() {
    use std::cell::RefCell;
    use std::sync::Mutex;

    let lock = Mutex::new(String::from("hello"));
    let inner_str = lock.into_inner().unwrap();
    assert_eq!(inner_str, "hello");

    let cell = RefCell::new(42);
    assert_eq!(cell.into_inner(), 42);

    let mut iter = vec![1, 2, 3].into_iter().peekable();
    assert_eq!(iter.peek(), Some(&1));
    let item = iter.next().unwrap(); // std 无 into_item；peek 后用 next
    assert_eq!(item, 1);

    println!("01-2-1 into_inner ok");
}
