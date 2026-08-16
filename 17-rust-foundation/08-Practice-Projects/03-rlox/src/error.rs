use std::fmt;

#[derive(Debug)]
pub struct LoxError {
    pub message: String,
    pub line: usize,
}

impl LoxError {
    pub fn new(line: usize, message: impl Into<String>) -> Self {
        Self {
            line,
            message: message.into(),
        }
    }
}

impl fmt::Display for LoxError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "[line {}] Error: {}", self.line, self.message)
    }
}

impl std::error::Error for LoxError {}

impl From<std::io::Error> for LoxError {
    fn from(e: std::io::Error) -> Self {
        Self {
            line: 0,
            message: e.to_string(),
        }
    }
}
