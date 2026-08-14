pub struct EmailClient {
    pub sender: String,
}

impl EmailClient {
    pub fn new(sender: impl Into<String>) -> Self {
        Self {
            sender: sender.into(),
        }
    }
}
