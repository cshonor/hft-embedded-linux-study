//! 库边界：具体 Error enum + thiserror

use thiserror::Error;

#[derive(Debug, Error, PartialEq)]
pub enum ConfigError {
    #[error("missing field `{0}`")]
    MissingField(&'static str),
    #[error("invalid port {0}")]
    InvalidPort(u16),
}

pub fn parse_port(s: &str) -> Result<u16, ConfigError> {
    let n: u16 = s.parse().map_err(|_| ConfigError::InvalidPort(0))?;
    if n == 0 {
        Err(ConfigError::InvalidPort(n))
    } else {
        Ok(n)
    }
}
