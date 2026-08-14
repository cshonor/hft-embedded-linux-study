use webserver_layout_demo::routes::handler_name;

#[test]
fn health_check_is_registered() {
    assert_eq!(handler_name(), "health_check");
}
