pub fn run() {
    let s = String::from("hello");
    let s1 = s.as_str();
    let s2 = s.as_str();
    assert_eq!(s, "hello");
    assert_eq!(s1, "hello");
    assert_eq!(s2, "hello");

    let x = Some(10);
    let r1 = x.as_ref();
    let r2 = x.as_ref();
    assert!(x.is_some());
    assert_eq!(r1, r2);

    println!("01-1 as_ ok");
}
