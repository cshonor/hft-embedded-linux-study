//! Item 4 拓展：`core::error::Error` + `no_std` + `alloc`

#![cfg_attr(not(test), no_std)]

#[cfg(not(test))]
extern crate alloc;

use core::error::Error;
use core::fmt::{self, Display, Formatter};

#[derive(Debug, PartialEq)]
pub enum PortError {
    Invalid(u16),
    Missing,
}

impl Display for PortError {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        match self {
            PortError::Invalid(p) => write!(f, "invalid port {p}"),
            PortError::Missing => write!(f, "port missing"),
        }
    }
}

impl Error for PortError {}

pub fn parse_port(s: &str) -> Result<u16, PortError> {
    if s.is_empty() {
        return Err(PortError::Missing);
    }
    let n: u16 = s.parse().map_err(|_| PortError::Invalid(0))?;
    if n == 0 {
        Err(PortError::Invalid(n))
    } else {
        Ok(n)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_trait_source_none() {
        let e = PortError::Missing;
        assert!(Error::source(&e).is_none());
        assert_eq!(e.to_string(), "port missing");
    }
}
