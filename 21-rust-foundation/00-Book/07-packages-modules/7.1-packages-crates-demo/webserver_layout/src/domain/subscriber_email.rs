#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SubscriberEmail(String);

impl SubscriberEmail {
    pub fn parse(email: impl Into<String>) -> Self {
        Self(email.into())
    }

    pub fn as_str(&self) -> &str {
        &self.0
    }
}
