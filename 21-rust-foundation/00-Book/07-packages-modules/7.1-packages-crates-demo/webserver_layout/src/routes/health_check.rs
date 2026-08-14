use crate::domain::{
    email_client::EmailClient,
    new_subscriber::NewSubscriber,
    subscriber_email::SubscriberEmail,
    subscriber_name::SubscriberName,
};

pub fn handler_name() -> &'static str {
    "health_check"
}

pub fn log_ready() {
    let _client = EmailClient::new("noreply@example.com");
    let _sub = NewSubscriber::new(
        SubscriberName::parse("alice"),
        SubscriberEmail::parse("alice@example.com"),
    );
    println!("[routes] {} ready (domain 子模块已链接)", handler_name());
}
