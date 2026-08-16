#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SubscriberName(String);

impl SubscriberName {
    pub fn parse(name: impl Into<String>) -> Self {
        Self(name.into())
    }

    pub fn as_str(&self) -> &str {
        &self.0
    }
}
