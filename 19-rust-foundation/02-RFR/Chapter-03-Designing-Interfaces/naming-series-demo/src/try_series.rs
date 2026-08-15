pub fn run() {
    use std::sync::Mutex;

    let lock = Mutex::new(5);
    let mut guard = lock.lock().unwrap();
    *guard = 10;
    assert!(lock.try_lock().is_err());
    drop(guard);

    let guard2 = lock.try_lock().expect("lock should be free");
    assert_eq!(*guard2, 10);

    let n: i32 = 42;
    let _big: i64 = n.try_into().unwrap();
    let huge: i128 = 1_000_000_000_000;
    let small: Result<i8, _> = huge.try_into();
    assert!(small.is_err());

    println!("01-4 try_ ok");
}
