pub fn run() {
    let v = vec![1, 2, 3];
    assert_eq!(v.get(1), Some(&2));
    assert_eq!(v.get(5), None);

    let mut w = vec![10, 20, 30];
    if let Some(x) = w.get_mut(0) {
        *x += 1;
    }
    assert_eq!(w, vec![11, 20, 30]);

    println!("01-3 get_ ok");
}
