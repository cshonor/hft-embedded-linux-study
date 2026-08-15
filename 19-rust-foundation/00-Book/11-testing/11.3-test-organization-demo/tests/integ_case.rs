use test_organization_demo::pub_add;

mod common;
use common::helper;

#[test]
fn test_inte_pub() {
    assert_eq!(pub_add(10, 20), 30);
}

#[test]
fn use_common() {
    assert_eq!(helper(), 10);
}
