pub fn run() {
    let v = vec![1, 2, 3];
    let mut v_iter = v.into_iter();
    assert_eq!(v_iter.next(), Some(1));
    assert_eq!(v_iter.next(), Some(2));
    assert_eq!(v_iter.next(), Some(3));
    assert_eq!(v_iter.next(), None);

    let s = String::from("hi");
    let bytes = s.into_bytes();
    assert_eq!(bytes, b"hi");

    println!("01-2 into_ ok");
}
